Buzzer & LEDs

Buzzer (+) → GPIO 4

Buzzer (–) → GND

LEDs:

LED color	ESP32 GPIO
Status/idle	2
Cleaning	15
Alert	0

Each LED:

Anode (+ long leg) → 220Ω resistor → GPIO pin

Cathode (– short leg) → GND

If you ever see boot issues, temporarily unplug LEDs on GPIO0/2/15 while flashing. For normal running they’re fine.