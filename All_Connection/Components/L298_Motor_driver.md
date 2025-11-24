Motors via L298N

Left motor: connected to OUT1 & OUT2 on L298N

Right motor: connected to OUT3 & OUT4

Control pins:

L298N pin	ESP32 GPIO	Use
IN1	16	Left motor direction 1
IN2	17	Left motor direction 2
IN3	18	Right motor direction 1
IN4	19	Right motor direction 2


ENA	23 (or jumper to 5V)	Left speed (PWM)
ENB	5 (or jumper to 5V)	Right speed (PWM)
(ENA, ENB jumpered to 5V on L298N for full speed.)
If you don’t want speed control at first:

Just put jumpers from ENA → 5V and ENB → 5V and don’t connect them to ESP32.