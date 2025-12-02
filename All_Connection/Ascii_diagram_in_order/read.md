                     ┌──────────────────────────────────────────┐
                     │                FRONT SIDE                 │
                     │                                          │
                     │        [ ULTRASONIC SENSOR ]             │
                     │        TRIG→5      ECHO→18               │
                     │                |                          │
                     │           [ WIPER SERVO ]  (PCA CH6)      │
                     ├──────────────────────────────────────────┤
                     │                                          │
                     │            [ 16x2 LCD DISPLAY ]          │
                     │        SDA→21   SCL→22   5V/GND           │
                     │                                          │
                     │ Water  Med   Help   Cancel  (Buttons)     │
                     │ 32     33     34      35                 │
                     ├──────────────────────────────────────────┤
                     │                                          │
                     │        ┌────────────────────────┐         │
                     │        │     PCA9685 BOARD      │         │
                     │        │                        │         │
                     │        │ SDA→21  SCL→22         │         │
                     │        │ VCC→3.3V  V+→5V        │         │
                     │        └────────────────────────┘         │
                     │         |   |   |   |   |   |   |         │
                     │         |   |   |   |   |   |   |         │
                     │   CH0→Waist   CH3→WristRoll  CH6→Wiper    │
                     │   CH1→Shoulder CH4→WristPitch             │
                     │   CH2→Elbow     CH5→Gripper               │
                     │   (All servo Brown→GND, Red→5V, Sig→CHx)  │
                     │                                          │
                     ├──────────────────────────────────────────┤
                     │                                          │
                     │    ┌───────────────────────────────┐     │
                     │    │           ESP32 DEVKIT        │     │
                     │    │ SDA(21)──┐                     │     │
                     │    │ SCL(22)──┴──> PCA + LCD + RTC │     │
                     │    │ 5V ← Buck Converter           │     │
                     │    │ GND ──────────────────────┐   │     │
                     │    │ Motor Pins → L298N         │   │     │
                     │    └───────────────────────────────┘     │
                     │                                          │
                     ├──────────────────────────────────────────┤
                     │                                          │
                     │        ┌────────────────────────┐         │
                     │        │        L298N           │         │
                     │        │                        │         │
                     │        │ OUT1/OUT2 → Left Motor │         │
                     │        │ OUT3/OUT4 → Right Motor│         │
                     │        │ ENA→14  IN1→26 IN2→27  │         │
                     │        │ ENB→25  IN3→32 IN4→33  │         │
                     │        │ GND ───────────────────┘         │
                     │        │ 12V ← Battery (via Switch)       │
                     │        └────────────────────────┘         │
                     │                                          │
                     ├──────────────────────────────────────────┤
                     │                                          │
                     │  [ BUCK CONVERTER ]                      │
                     │  IN+ ← Battery 7.4V  IN– ← Battery –     │
                     │  OUT+ → 5V → ESP32, PCA V+, LCD, RTC     │
                     │  OUT– → GND → All GND Common             │
                     │                                          │
                     │  [ BATTERY PACK 2×18650 = 7.4V ]          │
                     │   + → SWITCH → L298N + Buck               │
                     │   – → GND                                 │
                     │                                          │
                     └──────────────────────────────────────────┘
