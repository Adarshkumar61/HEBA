Libraries you must install (in Arduino IDE)

Before uploading the code, install:

ESP32 board support

File → Preferences → Additional Boards URL:
https://dl.espressif.com/dl/package_esp32_index.json

Then Tools → Board → Boards Manager → search ESP32 → install

ESP32Servo

Tools → Manage Libraries → search ESP32Servo → Install

LiquidCrystal_I2C

Tools → Manage Libraries → search LiquidCrystal I2C → Install
(If address 0x27 doesn’t work, try 0x3F.)

RTClib

Tools → Manage Libraries → search RTClib by Adafruit → Install