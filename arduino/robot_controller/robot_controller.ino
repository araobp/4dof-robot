#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <math.h>
#include <EEPROM.h>

/**
 * 物理パラメータ (単位: mm)
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
  int last_p[4]; // 最終パルス幅
} conf;

float curX = 150.0, curY = 0.0, curZ = 50.0; 
int current_us[4] = {1500, 1500, 1500, 1500}; 
bool moveLogEnabled = false; 

// 現在のパルス位置をEEPROMに保存
void saveLastPos() {
  for(int i=0; i<4; i++) conf.last_p[i] = current_us[i];
  EEPROM.put(0, conf);
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
      conf.last_p[i] = 1500;
    }
    conf.last_p[3] = 1500;
    conf.grip_open = 1000; conf.grip_close = 2000; conf.grip_speed_ms = 300;
    EEPROM.put(0, conf);
  }
  
  // ★ 記憶されたパルス幅をそのまま出力して静止開始
  for(int i=0; i<4; i++) {
    current_us[i] = conf.last_p[i];
    moveServo(i, current_us[i]);
  }

  Serial.println(F("--- ROBOT SYSTEM v3.5 (Static Startup) ---"));
  Serial.print(F("Initial Pulses: "));
  for(int i=0; i<4; i++) { Serial.print(current_us[i]); Serial.print(i<3?",":"\n"); }
  Serial.println(F("System Ready (No auto-move)."));
}

// 角度からパルスへの変換
int angleToUs(int ch, float angle) {
  float p0 = (float)conf.j_pulse[ch][0], p1 = (float)conf.j_pulse[ch][1];
  float a0 = conf.j_angle[ch][0], a1 = conf.j_angle[ch][1];
  if (abs(a1 - a0) < 0.01) return (int)p0; 
  return (int)(p0 + (angle - a0) * (p1 - p0) / (a1 - a0));
}

// サーボ物理駆動
void moveServo(int ch, int us) {
  if (ch < 0 || ch > 3) return;
  current_us[ch] = us;
  pwm.setPWM(ch, 0, (uint16_t)(us * 4096.0 / 20000.0));
}

// 逆運動学計算
bool calculateIK(float x, float y, float z, float &j1, float &j2, float &j3) {
  j1 = atan2(y, x) * 180.0 / PI;
  float r_j4 = sqrt(x*x + y*x) - L_OFF_J4_TCP - OFF_J1_J2;
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

// スムーズな座標移動
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
  saveLastPos(); // 完了後にEEPROM保存
}

void executeCommand(String cmd) {
  cmd.trim();
  if (cmd.startsWith("move")) {
    float tx = curX, ty = curY, tz = curZ, speed = 50.0;
    if(cmd.indexOf("x=") != -1) tx = cmd.substring(cmd.indexOf("x=")+2).toFloat();
    if(cmd.indexOf("y=") != -1) ty = cmd.substring(cmd.indexOf("y=")+2).toFloat();
    if(cmd.indexOf("z=") != -1) tz = cmd.substring(cmd.indexOf("z=")+2).toFloat();
    if(cmd.indexOf("s=") != -1) speed = cmd.substring(cmd.indexOf("s=")+2).toFloat();
    moveTo(tx, ty, tz, speed);
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
      Serial.println(F("Point registered."));
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
    saveLastPos();
  }
  else if (cmd.startsWith("c")) { 
    int eqIdx = cmd.indexOf('=');
    if (eqIdx != -1) {
      moveServo(cmd.substring(1, eqIdx).toInt(), cmd.substring(eqIdx + 1).toInt());
      saveLastPos();
    }
  }
  else if (cmd == "save") { saveLastPos(); Serial.println(F("Config Saved.")); }
  else if (cmd == "dump") {
    Serial.println(F("\n--- CONFIG DUMP ---"));
    for(int i=0; i<3; i++) {
      Serial.print("C"); Serial.print(i);
      Serial.print(": P0="); Serial.print(conf.j_pulse[i][0]); Serial.print(" A0="); Serial.print(conf.j_angle[i][0]);
      Serial.print(" | P1="); Serial.print(conf.j_pulse[i][1]); Serial.print(" A1="); Serial.print(conf.j_angle[i][1]);
      Serial.print("  CUR_PULSE="); Serial.println(current_us[i]);
    }
    Serial.println("--- END ---\n");
  }
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
  }
}