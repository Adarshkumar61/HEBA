// === HOSPITAL / ELDERLY / BACHELOR ASSISTANT ROBOT ===
// Brain: ESP32 DevKit
// Features: 5DOF arm, 2WD base, RTC schedule, LCD, buttons, buzzer, LEDs, HC-SR04 front sensor
// NO potentiometer, camera handled separately on ESP32-CAM board.

// ---------- Libraries ----------
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "RTClib.h"
#include <ESP32Servo.h>

// ---------- LCD & RTC ----------
#define LCD_ADDR 0x27      // If nothing shows, try 0x3F
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
RTC_DS3231 rtc;

// ---------- Pin definitions ----------
// Servos (5 DOF arm)
const int SERVO_BASE_PIN     = 13;
const int SERVO_SHOULDER_PIN = 14;
const int SERVO_ELBOW_PIN    = 25;
const int SERVO_WRIST_PIN    = 26;
const int SERVO_GRIPPER_PIN  = 27;

// L298N motor driver pins
const int IN1_PIN = 16;  // Left motor dir 1
const int IN2_PIN = 17;  // Left motor dir 2
const int IN3_PIN = 18;  // Right motor dir 1
const int IN4_PIN = 19;  // Right motor dir 2
// ENA / ENB are assumed jumpered to 5V for now (constant full speed)

// Buttons (using internal pull-ups)
const int BTN_WATER_PIN   = 32;
const int BTN_MED_PIN     = 33;
const int BTN_HELP_PIN    = 34;
const int BTN_CANCEL_PIN  = 35;

// Buzzer & LEDs
const int BUZZER_PIN      = 4;
const int LED_IDLE_PIN    = 2;
const int LED_CLEAN_PIN   = 15;
const int LED_ALERT_PIN   = 0;

// HC-SR04 ultrasonic
const int TRIG_PIN        = 12;  // output
const int ECHO_PIN        = 39;  // input-only pin (with voltage divider!)

// ---------- Servo objects ----------
Servo servoBase;
Servo servoShoulder;
Servo servoElbow;
Servo servoWrist;
Servo servoGripper;

// ---------- Robot state ----------
enum RobotState {
  STATE_IDLE,
  STATE_DO_WATER,
  STATE_DO_MEDICINE,
  STATE_DO_CLEANING,
  STATE_HELP_ALERT
};

RobotState currentState = STATE_IDLE;

// ---------- Schedule (change as you want) ----------
const int MED_HOUR    = 9;   // 09:00 = medicine
const int MED_MINUTE  = 0;

const int CLEAN_HOUR   = 10; // 10:00 = cleaning
const int CLEAN_MINUTE = 0;

int lastDay = -1;
bool medDoneToday = false;
bool cleanDoneToday = false;

// ---------- Button reading (simple debounce) ----------
bool lastWaterState = HIGH;
bool lastMedState   = HIGH;
bool lastHelpState  = HIGH;
bool lastCancelState= HIGH;
unsigned long lastButtonTime = 0;
const unsigned long debounceDelay = 50;

// ---------- Helper: small beep ----------
void beep(int durationMs = 150) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(durationMs);
  digitalWrite(BUZZER_PIN, LOW);
}

// ---------- Helper: set LEDs ----------
void setIdleLED(bool on)   { digitalWrite(LED_IDLE_PIN, on ? HIGH : LOW); }
void setCleanLED(bool on)  { digitalWrite(LED_CLEAN_PIN, on ? HIGH : LOW); }
void setAlertLED(bool on)  { digitalWrite(LED_ALERT_PIN, on ? HIGH : LOW); }

// ---------- Motor control helpers ----------
void motorsStop() {
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, LOW);
  digitalWrite(IN4_PIN, LOW);
}

void motorsForward() {
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, HIGH);
  digitalWrite(IN4_PIN, LOW);
}

void motorsBackward() {
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, HIGH);
  digitalWrite(IN3_PIN, LOW);
  digitalWrite(IN4_PIN, HIGH);
}

void motorsTurnLeft() {
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, HIGH);
  digitalWrite(IN3_PIN, HIGH);
  digitalWrite(IN4_PIN, LOW);
}

void motorsTurnRight() {
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, LOW);
  digitalWrite(IN4_PIN, HIGH);
}

// Move for some ms (simple time-based motion)
void driveFor(void (*motionFn)(), unsigned long ms) {
  motionFn();
  delay(ms);
  motorsStop();
}

// ---------- HC-SR04 distance function ----------
float getDistanceCm() {
  // Trigger pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Read echo
  long duration = pulseIn(ECHO_PIN, HIGH, 25000); // timeout ~25ms (~4m)
  if (duration == 0) {
    // No echo detected
    return -1.0;
  }
  float distance = (duration / 2.0f) * 0.0343f; // speed of sound ~343m/s
  return distance;
}

// ---------- Arm pose helper ----------
// You will TUNE these angles after testing on your real arm.
void setArmPose(int baseAngle, int shoulderAngle, int elbowAngle,
                int wristAngle, int gripperAngle) {
  servoBase.write(baseAngle);
  servoShoulder.write(shoulderAngle);
  servoElbow.write(elbowAngle);
  servoWrist.write(wristAngle);
  servoGripper.write(gripperAngle);
  delay(500);  // give time to move
}

// Some pre-defined poses (you will tune for your hardware)
void armHomePose() {
  setArmPose(90, 90, 90, 90, 90);
}

void armCleanPose1() {
  // Example: extended forward slightly
  setArmPose(90, 70, 110, 90, 90);
}

void armCleanPose2() {
  // Wipe to left
  setArmPose(70, 70, 110, 90, 90);
}

void armCleanPose3() {
  // Wipe to right
  setArmPose(110, 70, 110, 90, 90);
}

void armWaterDeliverPose() {
  // Pose where water cup is presented to user
  setArmPose(100, 80, 100, 100, 80);
}

void armMedicinePose() {
  // Pose where medicine tray is presented
  setArmPose(80, 85, 100, 90, 80);
}

// ---------- Routines ----------

void doCleaningRoutine() {
  currentState = STATE_DO_CLEANING;
  setIdleLED(false);
  setAlertLED(false);
  setCleanLED(true);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CLEANING ROOM...");
  lcd.setCursor(0, 1);
  lcd.print("Please wait");

  beep(200); delay(100); beep(200);

  // Simple cleaning pattern: wipe left-right a few times
  armCleanPose1();
  for (int i = 0; i < 3; i++) {
    armCleanPose2();
    delay(700);
    armCleanPose3();
    delay(700);
  }
  armHomePose();

  motorsStop(); // ensure base not moving

  setCleanLED(false);
  setIdleLED(true);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Cleaning done");
  lcd.setCursor(0, 1);
  lcd.print("Robot: IDLE");
  delay(1500);

  currentState = STATE_IDLE;
}

void doMedicineRoutine() {
  currentState = STATE_DO_MEDICINE;
  setIdleLED(false);
  setAlertLED(false);
  setCleanLED(false);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MEDICINE TIME!");
  lcd.setCursor(0, 1);
  lcd.print("Please take dose");

  // Long beep sequence
  for (int i = 0; i < 3; i++) {
    beep(200);
    delay(200);
  }

  // Move arm to medicine pose
  armMedicinePose();
  delay(2000); // user "takes" medicine
  armHomePose();

  setIdleLED(true);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Medicine served");
  lcd.setCursor(0, 1);
  lcd.print("Robot: IDLE");
  delay(1500);

  currentState = STATE_IDLE;
}

void doWaterRoutine() {
  currentState = STATE_DO_WATER;
  setIdleLED(false);
  setAlertLED(false);
  setCleanLED(false);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WATER REQUESTED");
  lcd.setCursor(0, 1);
  lcd.print("Checking path");

  // Check obstacle before moving
  float d = getDistanceCm();
  Serial.print("Front distance: ");
  Serial.println(d);

  if (d > 0 && d < 15.0) { // obstacle closer than 15cm
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Obstacle ahead!");
    lcd.setCursor(0, 1);
    lcd.print("Cannot move");
    beep(200); delay(200); beep(200);
    delay(1500);

    setIdleLED(true);
    currentState = STATE_IDLE;
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Bringing water");
  lcd.setCursor(0, 1);
  lcd.print("Moving...");

  // Simulate moving to water table and back
  driveFor(motorsForward, 800);   // go forward to table
  delay(300);
  motorsStop();

  // Present water
  armWaterDeliverPose();
  delay(2000);  // time to "pick"/present
  armHomePose();

  driveFor(motorsBackward, 800);  // return to base position
  motorsStop();

  setIdleLED(true);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Water delivered");
  lcd.setCursor(0, 1);
  lcd.print("Robot: IDLE");
  delay(1500);

  currentState = STATE_IDLE;
}

void doHelpAlert() {
  currentState = STATE_HELP_ALERT;
  setIdleLED(false);
  setCleanLED(false);
  setAlertLED(true);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("NURSE NEEDED!");
  lcd.setCursor(0, 1);
  lcd.print("Room: ALERT");

  // Repeating beep until cancel
  for (int i = 0; i < 10; i++) {  // ~10 beeps or until cancel
    if (digitalRead(BTN_CANCEL_PIN) == LOW) {
      break;
    }
    beep(150);
    delay(300);
  }

  setAlertLED(false);
  setIdleLED(true);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Alert noted");
  lcd.setCursor(0, 1);
  lcd.print("Robot: IDLE");
  delay(1000);

  currentState = STATE_IDLE;
}

// ---------- Button handling ----------
void handleButtons() {
  unsigned long nowMs = millis();
  if (nowMs - lastButtonTime < debounceDelay) return;
  lastButtonTime = nowMs;

  bool waterState  = digitalRead(BTN_WATER_PIN);
  bool medState    = digitalRead(BTN_MED_PIN);
  bool helpState   = digitalRead(BTN_HELP_PIN);
  bool cancelState = digitalRead(BTN_CANCEL_PIN);

  // Active LOW (because INPUT_PULLUP)
  if (waterState == LOW && lastWaterState == HIGH && currentState == STATE_IDLE) {
    doWaterRoutine();
  }
  if (medState == LOW && lastMedState == HIGH && currentState == STATE_IDLE) {
    doMedicineRoutine();
  }
  if (helpState == LOW && lastHelpState == HIGH && currentState != STATE_HELP_ALERT) {
    doHelpAlert();
  }
  if (cancelState == LOW && lastCancelState == HIGH) {
    // Cancel alert or any routine (simple version)
    motorsStop();
    armHomePose();
    setIdleLED(true);
    setCleanLED(false);
    setAlertLED(false);
    currentState = STATE_IDLE;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Action cancelled");
    lcd.setCursor(0, 1);
    lcd.print("Robot: IDLE");
    delay(1000);
  }

  lastWaterState  = waterState;
  lastMedState    = medState;
  lastHelpState   = helpState;
  lastCancelState = cancelState;
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  delay(500);

  // Pins
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(IN3_PIN, OUTPUT);
  pinMode(IN4_PIN, OUTPUT);
  motorsStop();

  pinMode(BTN_WATER_PIN,   INPUT_PULLUP);
  pinMode(BTN_MED_PIN,     INPUT_PULLUP);
  pinMode(BTN_HELP_PIN,    INPUT_PULLUP);
  pinMode(BTN_CANCEL_PIN,  INPUT_PULLUP);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_IDLE_PIN, OUTPUT);
  pinMode(LED_CLEAN_PIN, OUTPUT);
  pinMode(LED_ALERT_PIN, OUTPUT);

  // HC-SR04
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT); // with external voltage divider

  setIdleLED(true);
  setCleanLED(false);
  setAlertLED(false);
  digitalWrite(BUZZER_PIN, LOW);

  // I2C
  Wire.begin(); // SDA=21, SCL=22 by default on ESP32

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Assistant Robot");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(1500);

  // RTC
  if (!rtc.begin()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RTC ERROR!");
    Serial.println("RTC not found!");
    while (1) { delay(1000); }
  }
  if (rtc.lostPower()) {
    // Set RTC to compile time if battery removed
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  DateTime now = rtc.now();
  lastDay = now.day();

  // Servos
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  servoBase.setPeriodHertz(50);
  servoShoulder.setPeriodHertz(50);
  servoElbow.setPeriodHertz(50);
  servoWrist.setPeriodHertz(50);
  servoGripper.setPeriodHertz(50);

  servoBase.attach(SERVO_BASE_PIN,    500, 2400);
  servoShoulder.attach(SERVO_SHOULDER_PIN, 500, 2400);
  servoElbow.attach(SERVO_ELBOW_PIN,  500, 2400);
  servoWrist.attach(SERVO_WRIST_PIN,  500, 2400);
  servoGripper.attach(SERVO_GRIPPER_PIN, 500, 2400);

  armHomePose();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Robot Ready");
  lcd.setCursor(0, 1);
  lcd.print("Robot: IDLE");
  delay(1000);
}

// ---------- Loop ----------
void loop() {
  DateTime now = rtc.now();

  // Reset daily schedule flags at midnight
  if (now.day() != lastDay) {
    lastDay = now.day();
    medDoneToday = false;
    cleanDoneToday = false;
  }

  // Show time & state on LCD (basic refresh every ~1s)
  static unsigned long lastDisplayUpdate = 0;
  if (millis() - lastDisplayUpdate > 1000 && currentState == STATE_IDLE) {
    lastDisplayUpdate = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    // Print time HH:MM:SS
    char buf[9];
    sprintf(buf, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    lcd.print("Time: ");
    lcd.print(buf);

    lcd.setCursor(0, 1);
    lcd.print("State: IDLE   ");
  }

  // Check schedule: Medicine
  if (!medDoneToday &&
      now.hour() == MED_HOUR &&
      now.minute() == MED_MINUTE &&
      now.second() == 0 &&
      currentState == STATE_IDLE) {
    doMedicineRoutine();
    medDoneToday = true;
  }

  // Check schedule: Cleaning
  if (!cleanDoneToday &&
      now.hour() == CLEAN_HOUR &&
      now.minute() == CLEAN_MINUTE &&
      now.second() == 0 &&
      currentState == STATE_IDLE) {
    doCleaningRoutine();
    cleanDoneToday = true;
  }

  // Handle buttons anytime
  handleButtons();

  delay(10);
}
