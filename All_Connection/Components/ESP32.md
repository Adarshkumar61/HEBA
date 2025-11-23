ESP32 pin plan (follow this so it doesn’t get confusing)

Use this mapping:

🔹 Servos (5 DOF arm)
Joint	Servo pin	ESP32 GPIO
Base rotate	S1	13
Shoulder	S2	14
Elbow	S3	25
Wrist	S4	26
Gripper	S5	27

All servo red wires → 5V rail
All servo brown/black wires → GND rail
All servo orange/yellow wires → their ESP32 GPIO above