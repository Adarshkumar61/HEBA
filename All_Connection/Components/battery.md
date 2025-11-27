| Part             | Pin on module | Connects to                                                                     |
| ---------------- | ------------- | ------------------------------------------------------------------------------- |
| Battery +        | —             | Switch → Buck VIN+ → L298N +12V                                                 |
| Battery –        | —             | L298N GND → Buck VIN– (COMMON GND)                                              |
| Buck OUT + (5V)  | —             | ESP32 5V, PCA9685 V+, Servos V+, LCD VCC, RTC VCC, Buzzer+, LEDs+               |
| Buck OUT – (GND) | —             | ESP32 GND, PCA9685 GND, Servo GND, LCD GND, RTC GND, L298N GND, Buzzer –, LED – |
