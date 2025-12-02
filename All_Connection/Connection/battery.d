[2 batteries]  8.4V 
|
|----> ----> L298N 12V input (motor supply) 5v not supply to L298N
|
|----> Buck Converter IN+ / IN–
       |
       ----> Buck OUT 5V → ESP32 5V
       ----> Buck OUT 5V → PCA9685 V+ (servo power)
       ----> Buck OUT 5V → LCD VCC
       ----> Buck OUT 5V → RTC VCC
       ----> Buck OUT 5V → Ultrasonic VCC
       ----> Buck OUT 5V → Buttons pull-ups (if any)
       ----> Buck OUT 5V → LEDs, Buzzer (if needed)
