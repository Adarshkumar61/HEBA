STEP TO CALIBRATE (DO EXACTLY THIS):
1️⃣ Connect ESP32 → PCA9685 using 4 wires

SDA, SCL, VCC, GND
SDA = GPIO 21
SCL = GPIO 22
2️⃣ Connect ONE servo to PCA9685 CH0
3️⃣ Upload this code to ESP32:
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

4️⃣ Servo will automatically move to EXACT 90°
5️⃣ NOW attach the servo horn straight

(Right angle position)

6️⃣ Then mount the servo into the arm bracket
7️⃣ Remove it → plug next servo → repeat.