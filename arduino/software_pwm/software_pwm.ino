/*
 * Serial Command LED Controller
 * 
 * Supported Commands:
 * - blink=0 or blink=1 : Disable/Enable blinking
 * - brightness=0 to 10 : Set LED brightness (Software PWM)
 * - interval=N         : Set blink interval in milliseconds
 * - status             : Returns current state
 * 
 * Responses:
 * - ok : Command processed successfully
 * - ng : Command failed or ignored
 * - blink=X,brightness=Y,interval=Z : Response to status command
 */

// Global variables controlled by Serial commands and used in ISR
volatile bool blinking = false;
volatile int brightness = 5; // 0-10
volatile int pwm_counter = 0;
volatile int blink_counter = 0;
volatile bool blink_state = true;
volatile int blink_interval = 200;

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

  // Setup Timer1 for 1ms interrupt (1kHz)
  // We use Timer1 because it's a 16-bit timer, allowing for precise timing.
  noInterrupts();
  TCCR1A = 0; // Clear control registers
  TCCR1B = 0;
  TCNT1 = 0;  // Initialize counter to 0
  // Calculate OCR1A (Output Compare Register) for 1kHz interrupt frequency:
  // Formula: (CPU Frequency / (Prescaler * Target Frequency)) - 1
  // (16,000,000 / (64 * 1,000)) - 1 = 249
  OCR1A = 249; 
  TCCR1B |= (1 << WGM12); // Turn on CTC (Clear Timer on Compare Match) mode
  TCCR1B |= (1 << CS11) | (1 << CS10); // Set Prescaler to 64
  TIMSK1 |= (1 << OCIE1A); // Enable timer compare interrupt
  interrupts();
}

// Interrupt Service Routine called every 1ms
ISR(TIMER1_COMPA_vect) {
  // PWM Logic
  // The ISR runs every 1ms.
  // We use a counter from 0-9 to create a 10-step PWM cycle.
  // This results in a PWM period of 10ms, or 100Hz frequency.
  pwm_counter++;
  if (pwm_counter >= 10) pwm_counter = 0;

  // Blink Logic
  blink_counter++;
  if (blink_counter >= blink_interval) {
    blink_counter = 0;
    blink_state = !blink_state;
  }

  // Control LED based on blink state and software PWM brightness
  if (blinking && blink_state) {
    digitalWrite(LED_BUILTIN, (pwm_counter < brightness) ? HIGH : LOW);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void loop() {
  // Check for incoming serial data
  if (Serial.available() > 0) {
    // Read the incoming line from Serial
    String input = Serial.readStringUntil('\n');
    input.trim(); // Remove whitespace/newlines

    bool processed = false; // Flag to track if a valid command was found

    // Parse "blink=" command
    // Format: blink=0 (off) or blink=1 (on)
    int blinkIdx = input.indexOf("blink=");
    if (blinkIdx != -1) {
      // Extract value after "blink="
      int val = input.substring(blinkIdx + 6).toInt();
      if (val == 0) { blinking = false; processed = true; }
      else if (val == 1) { blinking = true; processed = true; }
    }

    // Parse "brightness=" command
    // Format: brightness=0 to 10
    int brightIdx = input.indexOf("brightness=");
    if (brightIdx != -1) {
      // Extract value after "brightness="
      int val = input.substring(brightIdx + 11).toInt();
      if (val >= 0 && val <= 10) { brightness = val; processed = true; }
    }

    // Parse "interval=" command
    // Format: interval=N (milliseconds)
    int intervalIdx = input.indexOf("interval=");
    if (intervalIdx != -1) {
      // Extract value after "interval="
      int val = input.substring(intervalIdx + 9).toInt();
      if (val > 0) { blink_interval = val; processed = true; }
    }

    // Handle "status" command
    if (input.indexOf("status") != -1) {
      Serial.print("blink=");
      Serial.print(blinking ? 1 : 0);
      Serial.print(",brightness=");
      Serial.print(brightness);
      Serial.print(",interval=");
      Serial.println(blink_interval);
      processed = true;
    }

    // Send response based on processing result
    if (processed) {
      Serial.println("ok");
    } else {
      Serial.println("ng");
    }
  }
}
