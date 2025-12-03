POWER CONNECTION (MG996R SERVO SETUP)
From	              To
Buck OUT+ (5V)  	PCA V+
Buck OUT– (GND)	    PCA GND
Buck OUT– (GND)	    ESP32 GND
ESP32 3.3V	        PCA VCC
ESP32 GND	        PCA GND

MG996R → PCA CHANNEL MAPPING
Servo Name	      Servo Type	PCA Channel
Waist (Base)	    MG996R	    CH0
Shoulder	        MG996R	    CH1
Elbow	            MG996R	    CH2


INDIVIDUAL WIRING PER-SERVO
🔵 WAIST SERVO (MG996R → CH0)

MG996R Wire	      Connect To
Brown (GND)	       PCA CH0 GND
Red (5V)	       PCA CH0 V+ (Buck 5V)
Orange (Signal)	   PCA CH0 SIG

SHOULDER SERVO (MG996R → CH1)
MG996R Wire	    Connect To
Brown	        PCA CH1 GND
Red	            PCA CH1 V+
Orange	        PCA CH1 SIG

🔵 ELBOW SERVO (MG996R → CH2)
MG996R Wire	     Connect To
Brown	         PCA CH2 GND
Red	             PCA CH2 V+
Orange	         PCA CH2 SIG


ESP32 → PCA (I2C) CONNECTION
PCA    Pin ESP32 Pin

SDA	   GPIO 21
SCL    GPIO 22
VCC	   3.3V
GND	   GND