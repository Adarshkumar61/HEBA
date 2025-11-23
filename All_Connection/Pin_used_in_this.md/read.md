Pin mapping used in this code (match your wiring)

Servos (arm):

Base servo → GPIO 13

Shoulder → GPIO 14

Elbow → GPIO 25

Wrist → GPIO 26

Gripper → GPIO 27

Motors (L298N):

IN1 → GPIO 16

IN2 → GPIO 17

IN3 → GPIO 18

IN4 → GPIO 19

ENA, ENB → jumper to 5V (no speed control in v1 – simpler)

I2C (LCD + RTC):

SDA → GPIO 21

SCL → GPIO 22

Buttons (using INPUT_PULLUP):

Water → GPIO 32

Medicine → GPIO 33

Help → GPIO 34

Cancel → GPIO 35

(One side of button to GPIO, other side to GND.)

Buzzer & LEDs:

Buzzer → GPIO 4

LED_IDLE → GPIO 2

LED_CLEAN → GPIO 15

LED_ALERT → GPIO 0

Power:

ESP32 5V pin from buck 5V

All modules’ VCC from buck 5V

All GNDs connected together