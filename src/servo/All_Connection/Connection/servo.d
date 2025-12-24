

MG996R (High torque)

Waist
Shoulder
Elbow

✔ SG90 (Micro servos)

Wrist Roll
Wrist Pitch
Gripper

✔ Extra:
Wiper Servo at front

Servo wiring (same for all 7):
Brown/Black → GND (Buck → PCA → Servo)
Red         → 5V (Buck 5V → PCA V+ → Servo)
Orange      → Signal (PCA channel)

PCA9685 ke har channel (0–15) ke 3 pins hote hain:

| GND | V+ | Signal |
(Left)(Mid)(Right)   

Important order:
You MUST plug servos in this exact order:

PCA9685
CH0 → Waist MG996R
CH1 → Shoulder MG996R
CH2 → Elbow MG996R
CH3 → Wrist Roll SG90
CH4 → Wrist Pitch SG90
CH5 → Gripper SG90
CH6 → Wiper SG90 (front)

Wiring EACH SERVO to PCA (VERY CLEAR)
Servo wire color coding:

Servo Type	GND	V+	Signal
MG996R	Brown or Black	Red	Orange / Yellow
SG90	Brown	Red	Orange