#include <Servo.h>

// --- Pin Configuration ---
// The PWM pin you have connected the servo's signal wire to.
// On Arduino UNO, pins 9 and 10 are controlled by Timer1.
// Using pin 9 is a good choice.
const int SERVO_PIN = 9;

// --- Servo and Movement Configuration ---
Servo my_servo;
volatile int target_angle = 90;    // Target angle in degrees (0-180)
volatile float current_angle = 90; // Current angle, starts at the middle
volatile float velocity = 60.0;    // Movement speed in degrees per second

// --- Setup Function ---
void setup() {
  // Initialize Serial communication at 9600 baud
  Serial.begin(9600);
  Serial.println("Servo Controller Initialized.");
  Serial.println("Send commands like 'rotation=90' or 'velocity=50'");

  // Attach the servo. For SG90/MG90S, default pulse widths are usually fine.
  // If your servo doesn't reach the full 0-180 range, you can fine-tune
  // with my_servo.attach(SERVO_PIN, min_pulse_width, max_pulse_width);
  my_servo.attach(SERVO_PIN);
  my_servo.write((int)current_angle); // Move to initial position

  // --- Timer1 Configuration for Interrupt Service Routine (ISR) ---
  // We will set up Timer1 to trigger an interrupt at a fixed frequency (100Hz).
  // This allows us to update the servo position in small, regular steps.
  
  cli(); // Stop interrupts temporarily

  // Clear Timer1 control registers
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  // Set the frequency to 100Hz.
  // Arduino UNO CPU frequency is 16MHz. We'll use a prescaler of 256.
  // OCR1A = (F_CPU / (Prescaler * Target_Frequency)) - 1
  // OCR1A = (16,000,000 / (256 * 100)) - 1 = 624
  OCR1A = 624;

  // Turn on CTC (Clear Timer on Compare Match) mode.
  // This makes the timer reset when it reaches OCR1A.
  TCCR1B |= (1 << WGM12);

  // Set prescaler to 256.
  TCCR1B |= (1 << CS12);

  // Enable the timer compare interrupt.
  TIMSK1 |= (1 << OCIE1A);

  sei(); // Re-enable interrupts
}

// --- Interrupt Service Routine for Timer1 ---
// This function is called automatically by the hardware at 100Hz.
// It must be short and fast.
ISR(TIMER1_COMPA_vect) {
  // Check if the servo is already at the target position.
  // We compare the rounded integer values.
  if ((int)round(current_angle) == target_angle) {
    return; // Nothing to do, exit ISR
  }

  // Calculate the angle change for this time step (1/100th of a second)
  float step = velocity / 100.0;

  // Move current_angle towards target_angle
  if (current_angle < target_angle) {
    current_angle += step;
    // Prevent overshooting the target
    if (current_angle > target_angle) {
      current_angle = target_angle;
    }
  } else if (current_angle > target_angle) {
    current_angle -= step;
    // Prevent undershooting the target
    if (current_angle < target_angle) {
      current_angle = target_angle;
    }
  }
  
  // Write the new position to the servo.
  // A static variable is used to write only when the integer angle changes.
  // This can help reduce servo jitter and unnecessary work.
  static int last_written_angle = -1;
  int angle_to_write = (int)round(current_angle);

  if (angle_to_write != last_written_angle) {
    my_servo.write(angle_to_write);
    last_written_angle = angle_to_write;
  }
}

// --- Main Loop ---
// The main loop is now very simple: it just checks for serial commands.
void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim(); // Remove any leading/trailing whitespace
    parseCommand(command);
  }
}

// --- Command Parsing Function ---
void parseCommand(String cmd) {
  int eqIndex = cmd.indexOf('=');
  if (eqIndex == -1) {
    Serial.println("Invalid command format. Use 'key=value'.");
    return;
  }

  String key = cmd.substring(0, eqIndex);
  String valueStr = cmd.substring(eqIndex + 1);
  float value = valueStr.toFloat();

  if (key == "rotation") {
    // Constrain the angle to the valid servo range (0-180)
    int new_target = constrain((int)round(value), 0, 180);
    
    // Atomically update the shared variable used by the ISR
    cli();
    target_angle = new_target;
    sei();
    
    Serial.print("Setting target rotation to: ");
    Serial.println(new_target);

  } else if (key == "velocity") {
    // Constrain velocity to a positive, reasonable value (e.g., 1 to 1000 deg/s)
    float new_velocity = constrain(value, 1.0, 1000.0);
    
    // Atomically update the shared variable
    cli();
    velocity = new_velocity;
    sei();

    Serial.print("Setting velocity to: ");
    Serial.print(new_velocity);
    Serial.println(" deg/s");

  } else {
    Serial.print("Unknown command: ");
    Serial.println(key);
  }
}
