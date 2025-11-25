BATTERIES (7.4–8.4V)
         |
        SWITCH   ← your robot ON/OFF switch
         |
         +------------------> L298N “12V” input  (bo motors)
         |
         +------------------> Buck Converter (VIN+ / VIN-)
                                       |
                             5V Output from Buck Converter
                                       |
         -----------------------------------------------------------------
         |                  |               |                |          |
     ESP32 5V pin      All 5 Servos      LCD VCC           RTC VCC   HC-SR04
         |             (SG90 motors)    (I2C)              (I2C)      VCC
         |                  |               |                |          |
     ESP32 GND pin <--same GND------------same GND--------same GND---same GND



GROK:
Battery (7.4V–12V)
    ├──→ L298N motor power terminal (direct, more torque)
    └──→ Buck converter input
            └──→ Buck output 5V → powers:
                    • ESP32 VIN
                    • All 6 servos red wire
                    • L298N 5V logic pin
                    • LCD, RTC, sensors
All GNDs connected together → one big common ground