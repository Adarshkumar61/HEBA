What this code does (behavior):

Shows time and status on LCD.

At MED_HOUR:MED_MINUTE (e.g. 9:00) → runs medicine routine:

Beeps, shows message, moves arm to a “medicine position”.

At CLEAN_HOUR:CLEAN_MINUTE (e.g. 10:00) → runs cleaning routine:

Beeps, shows message, moves arm in a “cleaning wipe” pattern.

If Water button pressed:

Robot simulates moving to water spot (drives forward/back a bit), then moves arm.

If Medicine button pressed:

Runs medicine arm pose directly.

If Help button pressed:

Buzzer + ALERT LED + LCD: “NURSE NEEDED”.

Cancel button stops active routines / turns off alert.