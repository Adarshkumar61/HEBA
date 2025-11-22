Buck converter output:

Adjust it to 5.0 V with a multimeter.

1. Buck VOUT+ (5V) →

ESP32 5V pin

Servo power rail (+5V)

LCD VCC

RTC VCC

Buzzer + LEDs (via resistors)

L298N 5V pin (optional, if you want its logic from same 5V)

2. Buck VOUT– (GND) →

ESP32 GND

Servo GND rail

LCD GND

RTC GND

Buzzer GND, LED GND

L298N GND


imp. So there are two voltages in the system:

~7.4 V → only to L298N motor power

5 V → everything else

⚠️ Never power servos or ESP32 directly from raw 7.4 V.