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
