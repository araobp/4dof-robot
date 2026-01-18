#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// Create the driver object using the default address 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// Servo settings
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates

// We use scaled integers (microseconds * 1000) for smooth, fast ISR math.
// 0 deg = 544us, 180 deg = 2400us.
int min_pulse = 544;
int max_pulse = 2400;
float us_per_deg = (2400 - 544) / 180.0; // ~10.311
float current_velocity = 60.0; // Degrees per second
long current_pos_scaled; // Current position in microseconds * 1000
long target_pos_scaled;  // Target position in microseconds * 1000
long step_scaled;        // Step size in microseconds * 1000 per tick
int current_channel = 0; // Current PWM channel (0-15)

unsigned long last_update_time = 0;
String inputBuffer = "";

void setup() {
  Serial.begin(9600);
  Serial.println("PCA9685 Servo Controller");
  Serial.println("Send commands like 'rotation=90' or 'velocity=50'");
  Serial.println("Calibration: 'min_pulse=544', 'max_pulse=2400'");
  Serial.println("Channel: 'channel=0' (0-15)");

  pwm.begin();
  
  // The internal oscillator is around 27MHz
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);
  
  // Initialize position at 90 degrees
  float start_angle = 90.0;
  long start_us = min_pulse + (long)(start_angle * us_per_deg);
  current_pos_scaled = start_us * 1000;
  target_pos_scaled = current_pos_scaled;
  step_scaled = (long)(current_velocity * us_per_deg); 
  
  // Initial move
  // Calculate the number of ticks (0-4095) corresponding to the start pulse width
  uint16_t ticks = (uint16_t)(start_us * 4096.0 / (1000000.0 / SERVO_FREQ));
  pwm.setPWM(current_channel, 0, ticks);

  last_update_time = micros();
}

// --- Servo Update Function ---
// Called from loop() every 1000us (1000Hz)
// This function updates the servo position incrementally to achieve smooth velocity control.
void updateServoMovement() {
  long current = current_pos_scaled;
  long target = target_pos_scaled;

  // If we are already at the target, do nothing
  if (current == target) {
    return;
  }

  long step = step_scaled;

  // Move current position towards target by 'step' amount
  if (current < target) {
    current += step;
    if (current > target) {
      current = target;
      current = target; // Prevent overshooting
    }
  } else {
    current -= step;
    if (current < target) {
      current = target;
      current = target; // Prevent undershooting
    }
  }
  
  current_pos_scaled = current;

  // Convert back to microseconds (divide by 1000)
  int us = (int)(current / 1000);
  
  // Convert microseconds to PCA9685 12-bit ticks (0-4095)
  // Formula: ticks = us * 4096 / (1000000 / freq)
  // (1000000 / freq) is the period in microseconds.
  // 4096 is the resolution of the PCA9685.
  uint16_t ticks = (uint16_t)(us * 4096.0 / (1000000.0 / SERVO_FREQ));
  pwm.setPWM(current_channel, 0, ticks);
}

// --- Main Loop ---
// Handles servo updates and non-blocking serial communication.
void loop() {
  // Check if it is time to update the servo (1000Hz)
  // Using micros() ensures precise timing without blocking the processor
  if (micros() - last_update_time >= 1000) {
    last_update_time += 1000;
    updateServoMovement();
  }

  // Process incoming serial data one character at a time
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n') {
      // End of command line, parse the buffer
      inputBuffer.trim();
      parseCommand(inputBuffer);
      inputBuffer = "";
    } else {
      // Append character to buffer
      inputBuffer += c;
    }
  }
}

// --- Command Parsing Function ---
// Parses commands in the format "key=value"
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
    // Set target angle
    // Constrain the angle to the valid servo range (0-180)
    float angle = constrain(value, 0.0, 180.0);
    
    // Calculate target in scaled microseconds
    long new_target_scaled = (long)((min_pulse + angle * us_per_deg) * 1000);
    
    target_pos_scaled = new_target_scaled;
    
    Serial.print("Setting target rotation to: ");
    Serial.println(angle);

  } else if (key == "velocity") {
    // Set movement speed
    // Constrain velocity to a positive, reasonable value (e.g., 1 to 1000 deg/s)
    current_velocity = constrain(value, 1.0, 1000.0);
    
    // Calculate step size: (deg/sec * us/deg) / 1000Hz * 1000(scale)
    // The /1000Hz and *1000(scale) cancel out.
    long new_step_scaled = (long)(current_velocity * us_per_deg);

    step_scaled = new_step_scaled;

    Serial.print("Setting velocity to: ");
    Serial.print(current_velocity);
    Serial.println(" deg/s");

  } else if (key == "min_pulse") {
    // Calibration: Set minimum pulse width (0 degrees)
    min_pulse = (int)value;
    us_per_deg = (max_pulse - min_pulse) / 180.0;
    
    // Recalculate velocity step based on new range
    step_scaled = (long)(current_velocity * us_per_deg);
    
    Serial.print("Min pulse set to: "); Serial.println(min_pulse);
    
  } else if (key == "max_pulse") {
    // Calibration: Set maximum pulse width (180 degrees)
    max_pulse = (int)value;
    us_per_deg = (max_pulse - min_pulse) / 180.0;
    
    // Recalculate velocity step based on new range
    step_scaled = (long)(current_velocity * us_per_deg);
    
    Serial.print("Max pulse set to: "); Serial.println(max_pulse);

  } else if (key == "channel") {
    // Select active PWM channel
    int ch = (int)value;
    if (ch >= 0 && ch <= 15) {
      current_channel = ch;
      Serial.print("Selected channel: "); Serial.println(current_channel);
    } else {
      Serial.println("Invalid channel. Must be 0-15.");
    }

  } else {
    Serial.print("Unknown command: ");
    Serial.println(key);
  }
}