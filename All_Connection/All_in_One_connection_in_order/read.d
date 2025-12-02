
# Hardware Connection Chart - Organized by Component

🟥 1) POWER SYSTEM

| Component | Connects To |
|-----------|------------|
| 2× 18650 (7.4V) + | Switch → Buck IN+ + L298N 12V |
| 2× 18650 (7.4V) – | Buck IN– + L298N GND |
| Buck OUT+ (5V) | ESP32 5V, PCA V+, LCD VCC, RTC VCC, HC-SR04 VCC, LEDs, Buzzer |
| Buck OUT– | ESP32 GND, PCA GND, LCD GND, RTC GND, HC-SR04 GND (ALL GROUNDS COMMON) |

🟦 2) L298N MOTOR DRIVER

| L298N Pin | Connects To |
|-----------|------------|
| 12V | Battery + (via switch) |
| GND | Battery –, ESP32 GND, PCA GND |
| OUT1 | Left Motor Wire 1 |
| OUT2 | Left Motor Wire 2 |
| OUT3 | Right Motor Wire 1 |
| OUT4 | Right Motor Wire 2 |
| ENA | ESP32 GPIO14 (PWM) |
| IN1 | ESP32 GPIO26 |
| IN2 | ESP32 GPIO27 |
| ENB | ESP32 GPIO25 (PWM) |
| IN3 | ESP32 GPIO32 |
| IN4 | ESP32 GPIO33 |

🟩 3) PCA9685 SERVO DRIVER

| PCA Pin | Connects To |
|---------|------------|
| VCC | ESP32 3.3V |
| GND | ESP32 GND |
| SDA | ESP32 GPIO21 |
| SCL | ESP32 GPIO22 |
| V+ (Servo Power) | Buck 5V |

🟧 4) 7 SERVOS (MG996R + SG90)

| Servo Function | PCA Channel | Wire Connection |
|---|---|---|
| Waist MG996R | CH0 | Brown→GND, Red→V+, Orange→PWM0 |
| Shoulder MG996R | CH1 | Brown→GND, Red→V+, Orange→PWM1 |
| Elbow MG996R | CH2 | Brown→GND, Red→V+, Orange→PWM2 |
| Wrist Roll SG90 | CH3 | Brown→GND, Red→V+, Orange→PWM3 |
| Wrist Pitch SG90 | CH4 | Brown→GND, Red→V+, Orange→PWM4 |
| Gripper SG90 | CH5 | Brown→GND, Red→V+, Orange→PWM5 |
| Wiper SG90 | CH6 | Brown→GND, Red→V+, Orange→PWM6 |

🟪 5) ESP32 CORE I/O

| ESP32 Pin | Connects To |
|-----------|------------|
| GPIO21 | PCA SDA + LCD SDA + RTC SDA |
| GPIO22 | PCA SCL + LCD SCL + RTC SCL |
| GPIO5 | Ultrasonic TRIG |
| GPIO18 | Ultrasonic ECHO |
| 5V | Buck 5V OUT |
| GND | Common Ground |

🟫 6) ULTRASONIC SENSOR (HC-SR04)

| HC-SR04 Pin | Connects To |
|-----------|------------|
| VCC | Buck 5V |
| GND | GND |
| TRIG | ESP32 GPIO5 |
| ECHO | ESP32 GPIO18 |

🟨 7) LCD 16×2 I2C

| LCD Pin | Connects To |
|---------|------------|
| VCC | Buck 5V |
| GND | GND |
| SDA | ESP32 GPIO21 |
| SCL | ESP32 GPIO22 |

🟫 8) RTC DS3231

| RTC Pin | Connects To |
|---------|------------|
| VCC | Buck 5V |
| GND | GND |
| SDA | ESP32 GPIO21 |
| SCL | ESP32 GPIO22 |

🟦 9) BUTTONS (4 push buttons)

| Button Name | ESP32 Pin | Other Side |
|-----------|-----------|-----------|
| Water | GPIO32 | GND |
| Medicine | GPIO33 | GND |
| Help | GPIO34 | GND |
| Cancel | GPIO35 | GND |

🟩 10) LEDs (with 220Ω resistor)

| LED Color | ESP32 Pin | Other Side |
|-----------|-----------|-----------|
| Yellow (Idle) | GPIO2 | GND |
| Green (Clean) | GPIO4 | GND |
| Red (Alert) | GPIO16 | GND |

🟥 11) BUZZER

| Buzzer Pin | Connects To |
|-----------|------------|
| + | ESP32 GPIO15 |
| – | GND |
GND	GND
SDA	ESP32 GPIO21
SCL	ESP32 GPIO22

💥 DONE → This is the FULL PERFECT CONNECTION CHART