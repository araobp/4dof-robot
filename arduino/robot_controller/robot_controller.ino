#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <math.h>
#include <EEPROM.h>

/**
 * Physical parameters (Unit: mm)
 */
const float L1 = 80.0;          
const float L2 = 80.0;          
const float L_OFF_J4_TCP = 64.0; 
const float Z_OFF_J4_TCP = 8.0; 
const float OFF_J1_J2 = 12.0;    
const float BASE_H = 60.0;       

const int STEP_DELAY = 10;       
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
#define SERVO_FREQ 50

struct Config {
  int signature; 
  int j_pulse[3][2];   
  float j_angle[3][2]; 
  int grip_open;       
  int grip_close;      
  int grip_speed_ms; 
} conf;

float curX = 150.0, curY = 0.0, curZ = 50.0; 
int current_us[4] = {1500, 1500, 1500, 1500}; 

// Save current pulse position and config to EEPROM
void saveConfig() {
  EEPROM.put(0, conf);
}

// Servo physical drive
void moveServo(int ch, int us) {
  if (ch < 0 || ch > 3) return;
  current_us[ch] = us;
  pwm.setPWM(ch, 0, (uint16_t)(us * 4096.0 / 20000.0));
}

// Convert angle to pulse using linear interpolation
int angleToUs(int ch, float angle) {
  float p0 = (float)conf.j_pulse[ch][0], p1 = (float)conf.j_pulse[ch][1];
  float a0 = conf.j_angle[ch][0], a1 = conf.j_angle[ch][1];
  if (abs(a1 - a0) < 0.01) return (int)p0; 
  return (int)(p0 + (angle - a0) * (p1 - p0) / (a1 - a0));
}

bool calculateIK(float x, float y, float z, float &j1, float &j2, float &j3) {
  j1 = atan2(y, x) * 180.0 / PI;
  float r_total = sqrt(x*x + y*y);
  float r_j4 = r_total - L_OFF_J4_TCP - OFF_J1_J2;
  float z_j4 = (z + Z_OFF_J4_TCP) - BASE_H;
  float s_sq = r_j4*r_j4 + z_j4*z_j4;
  float s = sqrt(s_sq);
  
  if (s > (L1 + L2) || s < abs(L1 - L2)) return false;

  float t3 = acos((L1*L1 + L2*L2 - s_sq) / (2.0 * L1 * L2)) * 180.0 / PI;
  float t2 = acos((L1*L1 + s_sq - L2*L2) / (2.0 * L1 * s)) * 180.0 / PI;
  
  j2 = atan2(z_j4, r_j4) * 180.0 / PI + t2;
  j3 = t3 + j2; 
  return true;
}

void moveTo(float tx, float ty, float tz, float speed) {
  float sx = curX, sy = curY, sz = curZ;
  float dist = sqrt(sq(tx - sx) + sq(ty - sy) + sq(tz - sz));
  int steps = max(1, (int)((dist / max(1.0f, speed)) * 1000.0 / STEP_DELAY));
  
  for (int i = 1; i <= steps; i++) {
    float t = (float)i / steps;
    float easedT = t * t * (3.0 - 2.0 * t);
    float j1, j2, j3;
    if (calculateIK(sx+(tx-sx)*easedT, sy+(ty-sy)*easedT, sz+(tz-sz)*easedT, j1, j2, j3)) {
      moveServo(0, angleToUs(0, j1)); 
      moveServo(1, angleToUs(1, j2)); 
      moveServo(2, angleToUs(2, j3));
      delay(STEP_DELAY);
    }
  }
  curX = tx; curY = ty; curZ = tz;
}

void executeCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  if (cmd.startsWith("move")) {
    float tx = curX, ty = curY, tz = curZ, speed = 50.0;
    if(cmd.indexOf("x=") != -1) tx = cmd.substring(cmd.indexOf("x=")+2).toFloat();
    if(cmd.indexOf("y=") != -1) ty = cmd.substring(cmd.indexOf("y=")+2).toFloat();
    if(cmd.indexOf("z=") != -1) tz = cmd.substring(cmd.indexOf("z=")+2).toFloat();
    if(cmd.indexOf("s=") != -1) speed = cmd.substring(cmd.indexOf("s=")+2).toFloat();
    moveTo(tx, ty, tz, speed);
  } 
  else if (cmd.startsWith("calibg")) {
    if (cmd.indexOf("open") != -1) {
      conf.grip_open = current_us[3];
      Serial.print(F("Grip OPEN registered: ")); Serial.println(conf.grip_open);
    } else if (cmd.indexOf("close") != -1) {
      conf.grip_close = current_us[3];
      Serial.print(F("Grip CLOSE registered: ")); Serial.println(conf.grip_close);
    } else {
      Serial.println(F("Usage: calibg <open|close>"));
    }
  }
  else if (cmd.startsWith("calib")) {
    int ptIdx = cmd.substring(5,6).toInt();
    float tx=0, ty=0, tz=0;
    if(cmd.indexOf("x=") != -1) tx = cmd.substring(cmd.indexOf("x=")+2).toFloat();
    if(cmd.indexOf("y=") != -1) ty = cmd.substring(cmd.indexOf("y=")+2).toFloat();
    if(cmd.indexOf("z=") != -1) tz = cmd.substring(cmd.indexOf("z=")+2).toFloat();
    float tj1, tj2, tj3;
    if (calculateIK(tx, ty, tz, tj1, tj2, tj3)) {
      conf.j_pulse[0][ptIdx] = current_us[0]; conf.j_angle[0][ptIdx] = tj1;
      conf.j_pulse[1][ptIdx] = current_us[1]; conf.j_angle[1][ptIdx] = tj2;
      conf.j_pulse[2][ptIdx] = current_us[2]; conf.j_angle[2][ptIdx] = tj3; 
      Serial.print(F("Point ")); Serial.print(ptIdx); Serial.println(F(" IK registered."));
    }
  }
  else if (cmd.startsWith("grip")) {
    int start_us = current_us[3];
    int target_us = (cmd.indexOf("open") != -1) ? conf.grip_open : conf.grip_close;
    int steps = max(1, conf.grip_speed_ms / STEP_DELAY);
    for (int i = 1; i <= steps; i++) {
      moveServo(3, start_us + (int)((target_us - start_us) * (float)i / steps));
      delay(STEP_DELAY);
    }
  }
  else if (cmd.startsWith("delay")) {
    int t = 1;
    if(cmd.indexOf("t=") != -1) t = cmd.substring(cmd.indexOf("t=")+2).toInt();
    delay(t);
  }
  else if (cmd.startsWith("c")) { 
    int eqIdx = cmd.indexOf('=');
    if (eqIdx != -1) {
      int ch = cmd.substring(1, eqIdx).toInt();
      int us = cmd.substring(eqIdx + 1).toInt();
      moveServo(ch, us);
    }
  }
  else if (cmd == "save") { 
    saveConfig(); 
    Serial.println(F("Config Saved to EEPROM.")); 
  }
  else if (cmd == "dump") {
    Serial.println(F("\n--- CONFIG DUMP ---"));
    for(int i=0; i<3; i++) {
      Serial.print("Ch"); Serial.print(i);
      Serial.print(": [P0="); Serial.print(conf.j_pulse[i][0]);
      Serial.print(", A0="); Serial.print(conf.j_angle[i][0], 1);
      Serial.print("] [P1="); Serial.print(conf.j_pulse[i][1]);
      Serial.print(", A1="); Serial.print(conf.j_angle[i][1], 1);
      Serial.print("] | CUR="); Serial.println(current_us[i]);
    }
    Serial.print(F("Grip: Open=")); Serial.print(conf.grip_open);
    Serial.print(F(", Close=")); Serial.print(conf.grip_close);
    Serial.print(F(" | CUR=")); Serial.println(current_us[3]);
    Serial.print(F("Current Logic TCP: X=")); Serial.print(curX);
    Serial.print(F(" Y=")); Serial.print(curY);
    Serial.print(F(" Z=")); Serial.println(curZ);
    Serial.println(F("-------------------\n"));
  }
  else if (cmd == "help") {
    Serial.println(F("\n--- COMMAND HELP ---"));
    Serial.println(F("move x=.. y=.. z=.. s=.. : Coordinate move"));
    Serial.println(F("c<ch>=<us>               : Direct drive (c3=1500)"));
    Serial.println(F("calib<0|1> x=.. y=.. z=..: IK calibration"));
    Serial.println(F("calibg <open|close>      : Gripper calibration"));
    Serial.println(F("grip <open|close>        : Gripper move"));
    Serial.println(F("delay t=<ms>             : Wait for t ms"));
    Serial.println(F("save                     : Write to EEPROM"));
    Serial.println(F("dump                     : Show current status"));
    Serial.println(F("help                     : This message"));
    Serial.println(F("--------------------\n"));
  }
  else {
    Serial.print(F("Unknown Command: ")); Serial.println(cmd);
  }
}

void setup() {
  Serial.begin(9600);
  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);

  EEPROM.get(0, conf);
  if (conf.signature != 0xABCD) {
    conf.signature = 0xABCD;
    for(int i=0; i<3; i++) {
      conf.j_pulse[i][0] = 1500; conf.j_angle[i][0] = 0.0;
      conf.j_pulse[i][1] = 2000; conf.j_angle[i][1] = 45.0;
    }
    conf.grip_open = 1000; conf.grip_close = 2000; conf.grip_speed_ms = 300;
    EEPROM.put(0, conf);
  }
  
  // Move to a safe starting position
  float j1, j2, j3;
  float startX = 130.0, startY = 0.0, startZ = 40.0;
  if (calculateIK(startX, startY, startZ, j1, j2, j3)) {
    moveServo(0, angleToUs(0, j1));
    moveServo(1, angleToUs(1, j2));
    moveServo(2, angleToUs(2, j3));
    curX = startX; curY = startY; curZ = startZ;
  } else {
    // Fallback to a safe position if IK fails
    for(int i=0; i<3; i++) moveServo(i, 1500);
  }
  moveServo(3, (conf.grip_open + conf.grip_close) / 2); // Gripper to neutral

  Serial.println(F("--- ROBOT SYSTEM v3.6 Ready ---"));
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    int startIdx = 0;
    int delimiterIdx = input.indexOf(';');
    while (delimiterIdx != -1) {
      executeCommand(input.substring(startIdx, delimiterIdx));
      startIdx = delimiterIdx + 1;
      delimiterIdx = input.indexOf(';', startIdx);
    }
    executeCommand(input.substring(startIdx));
    Serial.println(F("OK"));
  }
}