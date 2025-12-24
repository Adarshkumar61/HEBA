#include <ESP32Servo.h>

Servo s;

void setup() {
  s.attach(15);  // SG90 ka signal pin yaha connect hai
  s.write(90);   // Center position
}

void loop() {
}
