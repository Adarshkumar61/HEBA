                              Buck Converter 5V
                                    |
                           +--------+---------+
                           |        |         |
                           |        |         |
                     PCA V+ (middle pin row) |
                           |                 |
PCA GND (left row) <-------+-----------------+---> ESP32 GND
                           |
PCA VCC → ESP32 3.3V ------+

───────────────────────────────────────────────────────────────
PCA Channels (Servo Connections)
───────────────────────────────────────────────────────────────

CH0 (Waist MG996R)
   GND ← Brown wire
   V+  ← Red wire
   SIG ← Orange wire

CH1 (Shoulder MG996R)
   GND ← Brown
   V+  ← Red
   SIG ← Orange

CH2 (Elbow MG996R)
   GND ← Brown
   V+  ← Red
   SIG ← Orange

CH3 (Wrist Roll SG90)
   GND ← Brown
   V+  ← Red
   SIG ← Orange

CH4 (Wrist Pitch SG90)
   GND ← Brown
   V+  ← Red
   SIG ← Orange

CH5 (Gripper SG90)
   GND ← Brown
   V+  ← Red
   SIG ← Orange

CH6 (Wiper SG90)
   GND ← Brown
   V+  ← Red
   SIG ← Orange
