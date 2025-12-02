instruction: L298N ka 5V pin ko USE NAHI KARNA

Jumper remove mat karo, bas 12V par chalao

L298N ki logic internally 5V convert ho jayegi

Battery + → Switch → L298N 12V pin  
Battery – → L298N GND

Left motor wire 1 → OUT1  
Left motor wire 2 → OUT2

Right motor wire 1 → OUT3  
Right motor wire 2 → OUT4


LEFT MOTOR
ENA → ESP32 GPIO14   (PWM)
IN1 → ESP32 GPIO26
IN2 → ESP32 GPIO27

RIGHT MOTOR
ENB → ESP32 GPIO25   (PWM)
IN3 → ESP32 GPIO32
IN4 → ESP32 GPIO33

L298N Wiring Summary Table

L298N Pin	Connects To
12V	Battery + (via switch)
GND	Battery – + ESP32 GND + PCA GND
OUT1/OUT2	Left BO Motor
OUT3/OUT4	Right BO Motor
ENA	GPIO14
IN1	GPIO26
IN2	GPIO27
ENB	GPIO25
IN3	GPIO32
IN4	GPIO33
Important: L298N GROUND VERY IMPORTANT
L298N GND → ESP32 GND  
L298N GND → Buck GND  
L298N GND → PCA GND  
L298N GND → Battery –


All grounds MUST be common, warna motors unstable honge, servo jitter hoga.