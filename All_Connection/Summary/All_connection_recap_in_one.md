Connection summary table (all in one place)
Part	Pin on module	Connects to
Battery +	—	Switch → L298N 12V, Buck VIN+
Battery –	—	L298N GND, Buck VIN–
Buck VOUT+ (5V)	—	ESP32 5V, Servos V+, LCD VCC, RTC VCC, L298N 5V, Buzzer +, LEDs +
Buck VOUT– (GND)	—	ESP32 GND, Servos GND, LCD GND, RTC GND, L298N GND, Buzzer –, LED –
Servo S1 signal	Orange wire	ESP32 GPIO13
Servo S2 signal		GPIO14
Servo S3 signal		GPIO25
Servo S4 signal		GPIO26
Servo S5 signal		GPIO27
L298N IN1		GPIO16
L298N IN2		GPIO17
L298N IN3		GPIO18
L298N IN4		GPIO19
L298N ENA		GPIO23 or 5V
L298N ENB		GPIO5 or 5V
L298N OUT1/OUT2		Left BO motor
L298N OUT3/OUT4		Right BO motor
LCD SDA		GPIO21
LCD SCL		GPIO22
RTC SDA		GPIO21
RTC SCL		GPIO22
Button 1 (Water)	One side	GPIO32
	Other side	GND
Button 2 (Med)		GPIO33 ↔ GND
Button 3 (Help)		GPIO34 ↔ GND
Button 4 (Cancel)		GPIO35 ↔ GND
Buzzer +		GPIO4 via code
LED1 anode		GPIO2 via 220Ω
LED2 anode		GPIO15 via 220Ω
LED3 anode		GPIO0 via 220Ω