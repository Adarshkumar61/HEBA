#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>   // PCA9685
#include "RTClib.h"                    // DS3231
#include <LiquidCrystal_I2C.h>         // 16x2 LCD I2C

// -------------------- I2C Devices --------------------
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);  // PCA9685 default address
RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);     // change to 0x3F if needed

// -------------------- Servo Config --------------------
#define SERVO_MIN  110   // PCA tick for ~500µs (calibrate if needed)
#define SERVO_MAX  510   // PCA tick for ~2500µs

// PCA9685 Channels
#define SERVO_BASE      0
#define SERVO_SHOULDER  1
#define SERVO_ELBOW     2
#define SERVO_WRIST     3
#define SERVO_GRIPPER   4
#define SERVO_WIPER     5

// -------------------- Motor (L298N) -------------------
const int IN1 = 16;
const int IN2 = 17;
const int IN3 = 18;
const int IN4 = 19;
// ENA / ENB tied to 5V for full speed (or set as PWM pins if you want)

// -------------------- Ultrasonic ----------------------
const int TRIG_PIN = 26;
const int ECHO_PIN = 25;   // !!! Use 5V -> 3.3V divider on ECHO !!!

// -------------------- Buttons -------------------------
const int BTN_WATER_PIN   = 32;
const int BTN_MED_PIN     = 33;
const int BTN_HELP_PIN    = 34;  // input-only
const int BTN_CANCEL_PIN  = 35;  // input-only

// -------------------- Buzzer & LEDs -------------------
const int BUZZER_PIN      = 4;
const int LED_IDLE_PIN    = 2;   // boot pin
const int LED_CLEAN_PIN   = 15;  // boot pin
const int LED_ALERT_PIN   = 0;   // boot pin

// -------------------- Modes ---------------------------
enum RobotMode {
  MODE_IDLE,
  MODE_CLEANING,
  MODE_MEDICINE,
  MODE_HELP
};

RobotMode currentMode = MODE_CLEANING;  // start in cleaning mode

// -------------------- RTC Schedule --------------------
// Change these for your desired medicine time (24h format)
int medHour   = 9;
int medMinute = 0;
int lastMedDayRun = -1;   // to avoid multi-run in same day

// -------------------- Button state tracking -----------
// for edge detection
bool lastBtnWaterState  = HIGH;
bool lastBtnMedState    = HIGH;
bool lastBtnHelpState   = HIGH;
bool lastBtnCancelState = HIGH;

// Wiper oscillation
bool wiperDown = false;
unsigned long lastWiperToggle = 0;
const unsigned long WIPER_INTERVAL = 600; // ms

// Serial time print
unsigned long lastStatusPrint = 0;

// ------------------------------------------------------
// Servo helper: angle -> pulse
// ------------------------------------------------------
void setServoAngle(uint8_t ch, float angle) {
  if (angle < 0) angle = 0;
  if (angle > 180) angle = 180;
  uint16_t pulse = map((int)angle, 0, 180, SERVO_MIN, SERVO_MAX);
  pwm.setPWM(ch, 0, pulse);
}

// ------------------------------------------------------
// Ultrasonic distance
// ------------------------------------------------------
long getDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
  if (duration == 0) return 0;  // no echo
  long distance = duration * 0.034 / 2;
  return distance;
}

// ------------------------------------------------------
// Motor helpers
// ------------------------------------------------------
void motorsStop() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void motorsForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void motorsBackward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void motorsLeft() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void motorsRight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

// ------------------------------------------------------
// Buzzer helper
// ------------------------------------------------------
void beep(int t) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(t);
  digitalWrite(BUZZER_PIN, LOW);
}

// ------------------------------------------------------
// LED helper
// ------------------------------------------------------
void setLEDs(bool idleOn, bool cleanOn, bool alertOn) {
  digitalWrite(LED_IDLE_PIN,  idleOn  ? HIGH : LOW);
  digitalWrite(LED_CLEAN_PIN, cleanOn ? HIGH : LOW);
  digitalWrite(LED_ALERT_PIN, alertOn ? HIGH : LOW);
}

// ------------------------------------------------------
// Arm poses (YOU WILL TUNE THESE ANGLES)
// ------------------------------------------------------
void armHome() {
  setServoAngle(SERVO_BASE,      90);
  setServoAngle(SERVO_SHOULDER,  90);
  setServoAngle(SERVO_ELBOW,     90);
  setServoAngle(SERVO_WRIST,     90);
  setServoAngle(SERVO_GRIPPER,   40);   // closed
}

void armPickPose() {    // near medicine/water
  setServoAngle(SERVO_BASE,      120);
  setServoAngle(SERVO_SHOULDER,   70);
  setServoAngle(SERVO_ELBOW,     110);
  setServoAngle(SERVO_WRIST,      80);
}

void armServePose() {   // towards patient
  setServoAngle(SERVO_BASE,       60);
  setServoAngle(SERVO_SHOULDER,  110);
  setServoAngle(SERVO_ELBOW,      70);
  setServoAngle(SERVO_WRIST,      90);
}

void openGripper() {
  setServoAngle(SERVO_GRIPPER, 90);
}

void closeGripper() {
  setServoAngle(SERVO_GRIPPER, 40);
}

// Wiper control
void wiperDownPose() {
  setServoAngle(SERVO_WIPER, 60);
}

void wiperUpPose() {
  setServoAngle(SERVO_WIPER, 120);
}

// Oscillating wiper while cleaning
void updateWiper() {
  unsigned long now = millis();
  if (now - lastWiperToggle >= WIPER_INTERVAL) {
    lastWiperToggle = now;
    wiperDown = !wiperDown;
    if (wiperDown) wiperDownPose();
    else           wiperUpPose();
  }
}

// ------------------------------------------------------
// LCD helper
// ------------------------------------------------------
void lcdStatus(const char* line1, const char* line2 = "") {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

// ------------------------------------------------------
// Button edge detection
// ------------------------------------------------------
bool isButtonPressed(int pin, bool &lastStateRef) {
  bool reading = digitalRead(pin);
  bool pressed = false;
  if (lastStateRef == HIGH && reading == LOW) {
    pressed = true;  // falling edge
  }
  lastStateRef = reading;
  return pressed;
}

// ------------------------------------------------------
// Medicine routine
// ------------------------------------------------------
void runMedicineRoutine() {
  currentMode = MODE_MEDICINE;
  setLEDs(false, false, true);
  lcdStatus("Medicine Mode", "Serving...");

  motorsStop();
  wiperUpPose();
  beep(200);

  // Move forward little
  motorsForward();
  delay(800);
  motorsStop();

  // Pick
  armPickPose();
  delay(1200);
  openGripper();   // assume bottle present
  delay(600);
  closeGripper();
  delay(600);

  // Serve
  armServePose();
  delay(1500);

  // Return arm
  armHome();
  delay(800);

  // Move backwards
  motorsBackward();
  delay(800);
  motorsStop();

  lcdStatus("Medicine Done", "Robot Idle");
  setLEDs(true, false, false);
  currentMode = MODE_IDLE;
}

// ------------------------------------------------------
// Water routine (similar)
// ------------------------------------------------------
void runWaterRoutine() {
  currentMode = MODE_MEDICINE;
  setLEDs(false, false, true);
  lcdStatus("Water Mode", "Serving...");

  motorsStop();
  wiperUpPose();
  beep(200);

  motorsForward();
  delay(800);
  motorsStop();

  armPickPose();
  delay(1000);
  openGripper();
  delay(500);
  closeGripper();
  delay(500);

  armServePose();
  delay(1500);

  armHome();
  delay(800);

  motorsBackward();
  delay(800);
  motorsStop();

  lcdStatus("Water Done", "Robot Idle");
  setLEDs(true, false, false);
  currentMode = MODE_IDLE;
}

// ------------------------------------------------------
// HELP/SOS routine
// ------------------------------------------------------
void runHelpRoutine() {
  currentMode = MODE_HELP;
  motorsStop();
  wiperUpPose();
  setLEDs(false, false, true);
  lcdStatus("HELP CALLED!", "Beeping...");

  for (int i = 0; i < 10; i++) {
    beep(150);
    delay(150);
  }

  lcdStatus("HELP SIGNAL", "Waiting...");
}

// ------------------------------------------------------
// Cancel everything
// ------------------------------------------------------
void cancelAll() {
  motorsStop();
  armHome();
  wiperUpPose();
  setLEDs(true, false, false);
  lcdStatus("Cancelled", "Robot Idle");
  currentMode = MODE_IDLE;
}

// ------------------------------------------------------
// Cleaning behaviour (obstacle avoid)
// ------------------------------------------------------
void updateCleaningBehaviour() {
  long d = getDistanceCM();

  // Emergency stop range
  if (d > 0 && d <= 5) {
    motorsStop();
    wiperUpPose();
    setLEDs(false, false, true);
    lcdStatus("Obstacle Close", "Stopping!");
    return;
  }

  if (d > 5 && d < 20) {
    // Obstacle in front
    motorsStop();
    setLEDs(false, false, true);
    beep(100);

    motorsBackward();
    delay(300);
    motorsStop();

    motorsLeft();
    delay(400);
    motorsStop();

    setLEDs(false, true, false);
    lcdStatus("Cleaning Mode", "Avoiding...");
  } else {
    // Clear path
    setLEDs(false, true, false);
    motorsForward();
    updateWiper();
  }
}

// ------------------------------------------------------
// Check RTC for medicine schedule
// ------------------------------------------------------
void checkMedicineSchedule() {
  if (!rtc.begin()) return;

  DateTime now = rtc.now();

  if (millis() - lastStatusPrint >= 1000) {
    lastStatusPrint = millis();
    Serial.print("Time: ");
    Serial.print(now.hour());
    Serial.print(":");
    Serial.print(now.minute());
    Serial.print(":");
    Serial.println(now.second());
  }

  if (now.hour() == medHour && now.minute() == medMinute) {
    if (lastMedDayRun != now.day()) {
      lastMedDayRun = now.day();
      runMedicineRoutine();
    }
  }
}

// ------------------------------------------------------
// SETUP
// ------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Wire.begin();   // SDA=21, SCL=22 by default on ESP32

  // Motor pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  motorsStop();

  // Ultrasonic
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Buttons
  pinMode(BTN_WATER_PIN,   INPUT_PULLUP);
  pinMode(BTN_MED_PIN,     INPUT_PULLUP);
  pinMode(BTN_HELP_PIN,    INPUT_PULLUP);
  pinMode(BTN_CANCEL_PIN,  INPUT_PULLUP);

  // Buzzer & LEDs
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_IDLE_PIN, OUTPUT);
  pinMode(LED_CLEAN_PIN, OUTPUT);
  pinMode(LED_ALERT_PIN, OUTPUT);

  // PCA9685
  pwm.begin();
  pwm.setPWMFreq(50); // 50Hz for servos
  delay(10);

  // RTC
  if (!rtc.begin()) {
    Serial.println("RTC NOT FOUND!");
  }
  // 📌 Run ONCE to set time, then comment it:
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // LCD
  lcd.init();
  lcd.backlight();
  lcdStatus("HEBA Robot", "Starting...");

  // Initial arm + wiper
  armHome();
  wiperUpPose();

  setLEDs(false, true, false);
  currentMode = MODE_CLEANING;
  delay(1000);
  lcdStatus("Cleaning Mode", "Auto Start");
}

// ------------------------------------------------------
// LOOP
// ------------------------------------------------------
void loop() {
  // RTC-based schedule
  checkMedicineSchedule();

  // Buttons
  if (isButtonPressed(BTN_WATER_PIN, lastBtnWaterState)) {
    runWaterRoutine();
  }
  if (isButtonPressed(BTN_MED_PIN, lastBtnMedState)) {
    runMedicineRoutine();
  }
  if (isButtonPressed(BTN_HELP_PIN, lastBtnHelpState)) {
    runHelpRoutine();
  }
  if (isButtonPressed(BTN_CANCEL_PIN, lastBtnCancelState)) {
    cancelAll();
  }

  // Mode behaviour
  switch (currentMode) {
    case MODE_CLEANING:
      updateCleaningBehaviour();
      break;

    case MODE_IDLE:
      motorsStop();
      setLEDs(true, false, false);
      // no movement
      break;

    case MODE_MEDICINE:
      // handled inside routines
      break;

    case MODE_HELP:
      // just waiting after help
      break;
  }

  delay(40);  // small delay for stability
}
