#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define SERVO_MIN 110
#define SERVO_MAX 510

void setup() {
  pwm.begin();
  pwm.setPWMFreq(50);
  delay(500);

  // Move Channel 0 servo to 90 degrees
  int pulse = map(90, 0, 180, SERVO_MIN, SERVO_MAX);
  pwm.setPWM(0, 0, pulse); 
}

void loop() {}
