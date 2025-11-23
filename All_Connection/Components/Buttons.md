I2C bus (LCD + RTC)

Use ESP32 default I2C pins:

Device	Pin on module	ESP32 GPIO
LCD	SDA	21
LCD	SCL	22
RTC	SDA	21
RTC	SCL	22

LCD VCC → 5V

LCD GND → GND

RTC VCC → 3.3V or 5V (DS3231 works on both, I2C level is fine)

RTC GND → GND

SDA lines of LCD and RTC both tie together to GPIO21.
SCL lines of both tie together to GPIO22.