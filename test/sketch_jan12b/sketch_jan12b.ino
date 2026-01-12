#include <Servo.h>

Servo myservo;
const int MIN = 500;
const int MAX = 2400;
int duty = MIN;
int increase = 1;
const int INTERVAL = 10;

void setup() {
  myservo.attach(9);
  myservo.writeMicroseconds(duty);
}

void loop() {
  delay(INTERVAL);
  myservo.writeMicroseconds(duty);
  if (duty >= MAX) {
    increase = -10;
  } else if (duty <= MIN) {
    increase = 10;
  }
  duty = duty + increase;
}
