Step-by-step wiring order (so you don’t get lost)
🧩 Step 1 – Power & ESP32 only

Wire battery → switch → L298N + buck input.

Set buck output to exactly 5V.

Connect buck 5V → ESP32 5V, and buck GND → ESP32 GND.

Plug ESP32 to laptop and upload blinky LED to test power.

🧩 Step 2 – Add LCD + RTC (I2C bus)

Connect LCD + RTC I2C as in table.

Run a small test program: print time on LCD.

Confirms I2C and RTC working.

🧩 Step 3 – Add one servo first

Connect only base servo (S1) to 13, 5V, GND.

Run a servo test sketch (sweep).

If okay, connect remaining 4 servos one by one.

🧩 Step 4 – Add L298N + BO motors

Wire motors to OUT1, OUT2, OUT3, OUT4.

Wire IN1..IN4 to 16,17,18,19.

For first test, ENA/ENB → 5V.

Run a test: forward, backward, left, right.

🧩 Step 5 – Add buttons

Connect buttons to 32,33,34,35 with other leg to GND.

Test: print in Serial Monitor when each button is pressed.

🧩 Step 6 – Add buzzer + LEDs

Wire buzzer + LEDs.

Test: short beep function, status LED blink.