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


that code which is running: 
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// PCA9685 at default I2C address 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// MG996R on channel 0 (change if needed)
const uint8_t MG_CH = 0;

// MG996R ke liye thoda wide range rakhte hain.
// Agar servo thoda jada ya kam ghoome to in dono values ko tune kar sakta hai.
#define MG_SERVOMIN  130   // pulse count for ~0°
#define MG_SERVOMAX  600   // pulse count for ~180°

int angleToPulse(int angle) {
  if (angle < 0) angle = 0;
  if (angle > 180) angle = 180;
  return map(angle, 0, 180, MG_SERVOMIN, MG_SERVOMAX);
}

void setServoAngle(uint8_t ch, int angle) {
  int pulse = angleToPulse(angle);
  pwm.setPWM(ch, 0, pulse);
}

// Smooth move helper (optional – looks nice)
void moveSmooth(uint8_t ch, int fromAngle, int toAngle, int step = 2, int delayMs = 15) {
  if (fromAngle < toAngle) {
    for (int a = fromAngle; a <= toAngle; a += step) {
      setServoAngle(ch, a);
      delay(delayMs);
    }
  } else {
    for (int a = fromAngle; a >= toAngle; a -= step) {
      setServoAngle(ch, a);
      delay(delayMs);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // ESP32 I2C pins
  Wire.begin(21, 22);

  pwm.begin();
  pwm.setPWMFreq(50);  // 50Hz for servo
  delay(500);

  Serial.println("MG996R test start");

  // 1) Pehle seedha 90° pe le jao (mounting / centering ke liye)
  setServoAngle(MG_CH, 90);
  Serial.println("Centered at 90°");
  delay(2000);

  // 2) Test: 90° se 0° phir 180° phir wapas 90° smooth
  moveSmooth(MG_CH, 90, 0);
  delay(500);
  moveSmooth(MG_CH, 0, 180);
  delay(500);
  moveSmooth(MG_CH, 180, 90);
  delay(500);

  Serial.println("Test motion done, holding at 90°");
}

void loop() {
  // Kuch nahi – servo 90° pe hold karega
}
