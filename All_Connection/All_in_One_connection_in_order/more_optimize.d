🟩 🔥 FULL WIRING PATH SUMMARY (ASCII arrows)
🟦 I2C BUS
ESP32 GPIO21 (SDA) ────────> PCA9685 SDA
                       └───> LCD SDA
                       └───> RTC SDA

ESP32 GPIO22 (SCL) ────────> PCA9685 SCL
                       └───> LCD SCL
                       └───> RTC SCL

🟥 SERVO CONNECTIONS
Waist MG996R     → PCA CH0
Shoulder MG996R  → PCA CH1
Elbow MG996R     → PCA CH2
WristRoll SG90   → PCA CH3
WristPitch SG90  → PCA CH4
Gripper SG90     → PCA CH5
Wiper SG90       → PCA CH6

Each servo:
Brown → GND
Red   → V+ (5V buck)
Orange→ CHx Signal

🟧 ULTRASONIC
HC-SR04 TRIG → ESP32 GPIO5
HC-SR04 ECHO → ESP32 GPIO18
HC-SR04 VCC  → 5V
HC-SR04 GND  → GND

🟨 MOTOR DRIVER (L298N)
ENA → ESP32 GPIO14
IN1 → ESP32 GPIO26
IN2 → ESP32 GPIO27

ENB → ESP32 GPIO25
IN3 → ESP32 GPIO32
IN4 → ESP32 GPIO33

Motor OUT1/OUT2 → Left BO Motor
Motor OUT3/OUT4 → Right BO Motor

12V → Battery +
GND → Battery – + ESP32 GND + Buck GND

🟫 POWER SYSTEM
Battery + → Switch → L298N 12V
                        └→ Buck IN+

Battery – → L298N GND → ESP32 GND → PCA GND → LCD GND → RTC GND → Servo GND → HC-SR04 GND
                                     (ALL GROUNDS JOIN)


Buck OUT:

Buck 5V → ESP32 5V
         → PCA V+
         → LCD VCC
         → RTC VCC
         → Ultrasonic VCC
         → Servos (via PCA)

🟪 BUTTONS
Water Button  → GPIO32 → GND  
Med Button    → GPIO33 → GND  
Help Button   → GPIO34 → GND  
Cancel Button → GPIO35 → GND  

🟩 LEDs (with 220Ω resistor)
GPIO2 → Yellow LED → GND
GPIO4 → Green LED  → GND
GPIO16 → Red LED   → GND

🟦 BUZZER
ESP32 GPIO15 → Buzzer +
GND → Buzzer –
