| From | To | Wire Color | Notes |
|------|----|-----------:|-------|
| ESP32 5V | Buck converter 5V out | Red | ESP power |
| ESP32 GND | All GNDs | Black | Common |
| ESP32 GPIO21 (SDA) | PCA9685 SDA | White | I2C data |
| ESP32 GPIO22 (SCL) | PCA9685 SCL | Yellow | I2C clock |
| ESP32 GPIO27 | HC-SR04 Trig | Orange | Ultrasonic |
| ESP32 GPIO26 | HC-SR04 Echo | Brown | Ultrasonic |
| ESP32 GPIO14 | L298N IN1 (left motor) | Blue | Motor dir |
| ESP32 GPIO12 | L298N IN2 | Green | Motor dir |
| ESP32 GPIO13 | L298N IN3 (right motor) | Purple | Motor dir |
| ESP32 GPIO15 | L298N IN4 | Grey | Motor dir |
| PCA9685 VCC | ESP32 3.3V | Red thin | Logic power |
| PCA9685 V+ terminal | Buck 5-6V out | Thick red | Servo power (10A) |
| PCA9685 GND terminal | Common GND | Thick black | - |
| Battery 7.4V (+) | L298N 12V & Buck input | Thick red | Motors + servos |
| Battery 7.4V (-) | Common GND | Thick black | - |
| Servos signal (yellow) | PCA9685 channels 0-6 | - | Plug direct |
| Servos red | PCA9685 V+ | Thick red | - |
| Servos black | PCA9685 GND | Thick black | - |
| DS3231 SDA | ESP32 GPIO21 (shared) | White | RTC |
| DS3231 SCL | ESP32 GPIO22 (shared) | Yellow | RTC |


Buck converter set to 5.6V. No direct servo power from ESP32!