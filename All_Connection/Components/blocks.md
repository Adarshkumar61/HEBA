Think of 4 main “blocks”:

Power block – batteries → buck → 5V → all electronics
 
ESP32 block – brain → talks to everything

Motion block – 5 servos (arm) + 2 BO motors (wheels via L298N)

IO block – LCD, RTC, buttons, buzzer, LEDs, camera

All grounds (GND) of every block must be connected together.