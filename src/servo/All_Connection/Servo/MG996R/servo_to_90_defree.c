#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(0x40);

#define SERVO_MIN 120
#define SERVO_MAX 600

uint16_t pulse(int angle) {
  return map(angle, 0, 180, SERVO_MIN, SERVO_MAX);
}

void center(uint8_t ch) {
  pca.setPWM(ch, 0, pulse(90));
  delay(500);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Centering MG996R servos...");

  Wire.begin(21, 22);  
  pca.begin();
  pca.setPWMFreq(50);
  delay(10);

  // MG996R channels
  center(0); // Waist
  center(1); // Shoulder
  center(2); // Elbow

  Serial.println("MG996R centering done!");
}

void loop() {

}
