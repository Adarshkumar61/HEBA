#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>

// ========== WiFi ==========
const char* ssid     = "HEBA_Robot";
const char* password = "12345678";

WebServer server(80);

// ========== L298N + Motors ==========
#define ENA 14
#define IN1 26
#define IN2 27
#define IN3 32
#define IN4 33
#define ENB 25

// ========== Ultrasonic ==========
#define TRIG 5
#define ECHO 18
#define OBSTACLE_CM 25

// ========== LEDs ==========
#define LED_YELLOW 2
#define LED_GREEN  4
#define LED_RED    16

// ========== Servos / PCA9685 ==========
#define NUM_ARM_SERVOS 6   // arm joints
#define NUM_SERVOS     7   // arm + wiper

Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(0x40);  // PCA9685 default
RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);   // change to 0x3F if needed

// Servo channels on PCA9685
// 3× MG995
#define SERVO_BASE       0
#define SERVO_WAIST      1
#define SERVO_ARM2       2
// 3× SG90/MG90
#define SERVO_END_ARM2   3
#define SERVO_ARM3       4
#define SERVO_HOLDER     5
// Separate front wiper
#define SERVO_WIPER      6

// Servo PWM limits (tune if needed)
#define SERVO_MIN  120
#define SERVO_MAX  600

// Wiper timing (approx full cycle ~4s)
#define WIPER_STEP_DEG        3
#define WIPER_MIN_ANGLE       0
#define WIPER_MAX_ANGLE       180
#define WIPER_HALF_PERIOD_MS  2000UL            // 0→180 ~2s
#define WIPER_STEP_MS         (WIPER_HALF_PERIOD_MS * WIPER_STEP_DEG / 180)

// ===== Mode system =====
enum RobotMode {
  MODE_IDLE,
  MODE_CLEANING,
  MODE_WATER,
  MODE_MEDICINE,
  MODE_GARBAGE,
  MODE_OBSTACLE_STOP
};

RobotMode currentMode = MODE_IDLE;
RobotMode prevMode    = MODE_IDLE;

// ========== Teaching sequences ==========
#define MAX_FRAMES 20

struct Pose {
  uint8_t  servo[NUM_ARM_SERVOS];   // only 6 arm servos
  int16_t  leftSpeed;               // -255..255
  int16_t  rightSpeed;              // -255..255
  uint16_t durationMs;
};

Pose waterSeq[MAX_FRAMES];
int  waterLen = 0;

Pose medSeq[MAX_FRAMES];
int  medLen = 0;     

Pose garbageSeq[MAX_FRAMES];
int  garbageLen = 0;

Pose cleanSeq[MAX_FRAMES];
int  cleanLen = 0;

// Current manual control state
uint8_t currentServoAngles[NUM_SERVOS] = {90,90,90,90,90,90,90};
int16_t currentLeftSpeed  = 0;
int16_t currentRightSpeed = 0;

// Playback state
bool          playing        = false;
Pose*         playSeq        = nullptr;
int           playLen        = 0;
int           playIndex      = 0;
int           lastFrameIndex = -1;
unsigned long frameStartTime = 0;

// RTC scheduling
int lastScheduleMinute = -1;

// Wiper state (continuous sweep)
int           wiperAngle      = WIPER_MIN_ANGLE;
int           wiperDir        = +1;   // +1 up, -1 down
unsigned long lastWiperUpdate = 0;

// ========== Utility: map angle to PCA pulse ==========
uint16_t angleToPulse(uint8_t angle) {
  return map(angle, 0, 180, SERVO_MIN, SERVO_MAX);
}

void setServo(uint8_t ch, uint8_t angle) {
  if (ch >= NUM_SERVOS) return;
  currentServoAngles[ch] = angle;
  uint16_t pulse = angleToPulse(angle);
  pca.setPWM(ch, 0, pulse);
}

// ========== Motors control ==========
void setMotors(int16_t left, int16_t right) {
  currentLeftSpeed  = left;
  currentRightSpeed = right;

  // left
  if (left > 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    ledcWrite(0, left);   // channel 0 -> ENA
  } else if (left < 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    ledcWrite(0, -left);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    ledcWrite(0, 0);
  }

  // right
  if (right > 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    ledcWrite(1, right);  // channel 1 -> ENB
  } else if (right < 0) {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    ledcWrite(1, -right);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    ledcWrite(1, 0);
  }
}

void stopMotors() {
  setMotors(0, 0);
}

// ========== Ultrasonic distance ==========
long getDistanceCm() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, 30000);  // timeout ~30ms
  if (duration == 0) return 400; // no echo
  long cm = duration / 58;
  return cm;
}

// ========== LEDs by mode ==========
void updateLEDs() {
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);

  switch (currentMode) {
    case MODE_IDLE:
      digitalWrite(LED_YELLOW, HIGH);
      break;
    case MODE_CLEANING:
    case MODE_WATER:
    case MODE_MEDICINE:
    case MODE_GARBAGE:
      digitalWrite(LED_GREEN, HIGH);
      break;
    case MODE_OBSTACLE_STOP:
      digitalWrite(LED_RED, HIGH);
      break;
  }
}

// ========== LCD update ==========
void showStatus(const char* line1, const char* line2) {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(line1);
  lcd.setCursor(0,1);
  lcd.print(line2);
}

const char* modeToStr(RobotMode m) {
  switch (m) {
    case MODE_IDLE:          return "IDLE";
    case MODE_CLEANING:      return "CLEANING";
    case MODE_WATER:         return "WATER";
    case MODE_MEDICINE:      return "MEDICINE";
    case MODE_GARBAGE:       return "GARBAGE";
    case MODE_OBSTACLE_STOP: return "OBSTACLE";
  }
  return "UNKNOWN";
}

void updateLCD() {
  DateTime now = rtc.now();
  char line1[17];
  char line2[17];
  snprintf(line1, sizeof(line1), "%02d:%02d %s",
           now.hour(), now.minute(), modeToStr(currentMode));
  long d = getDistanceCm();
  snprintf(line2, sizeof(line2), "Dist:%3ldcm", d);
  showStatus(line1, line2);
}

// ========== Save current pose to sequence ==========
bool savePoseToSeq(Pose* seq, int &len, uint16_t durMs) {
  if (len >= MAX_FRAMES) return false;
  Pose &p = seq[len];
  for (int i=0;i<NUM_ARM_SERVOS;i++) p.servo[i] = currentServoAngles[i]; // 0..5 only
  p.leftSpeed  = currentLeftSpeed;
  p.rightSpeed = currentRightSpeed;
  p.durationMs = durMs;
  len++;
  return true;
}

// ========== Hardcoded DEMO cleaning sequence ==========
void initDemoCleaningSequence() {
  cleanLen = 0;
  Pose p;

  // Frame 0: neutral arm, robot still
  p.servo[0] = 90;   // base
  p.servo[1] = 90;   // waist
  p.servo[2] = 90;   // arm2
  p.servo[3] = 90;   // end arm2
  p.servo[4] = 90;   // arm3
  p.servo[5] = 60;   // holder (adjust)
  p.leftSpeed  = 0;
  p.rightSpeed = 0;
  p.durationMs = 800;
  cleanSeq[cleanLen++] = p;

  // Frame 1: slight forward move
  p.leftSpeed  = 140;
  p.rightSpeed = 140;
  p.durationMs = 1800;
  cleanSeq[cleanLen++] = p;

  // Frame 2: LEFT sweep
  p.leftSpeed  = -120;
  p.rightSpeed = 120;
  p.durationMs = 800;
  cleanSeq[cleanLen++] = p;

  // Frame 3: RIGHT sweep
  p.leftSpeed  = 120;
  p.rightSpeed = -120;
  p.durationMs = 800;
  cleanSeq[cleanLen++] = p;

  // Frame 4: Move back
  p.leftSpeed  = -140;
  p.rightSpeed = -140;
  p.durationMs = 1800;
  cleanSeq[cleanLen++] = p;

  // Frame 5: stop
  p.leftSpeed  = 0;
  p.rightSpeed = 0;
  p.durationMs = 1000;
  cleanSeq[cleanLen++] = p;
}

// ========== Start playing a sequence ==========
void startPlay(Pose* seq, int len, RobotMode mode) {
  if (len <= 0) return;
  playSeq        = seq;
  playLen        = len;
  playIndex      = 0;
  lastFrameIndex = -1;
  playing        = true;
  frameStartTime = millis();
  currentMode    = mode;
  updateLEDs();
}

// ========== Playback step (called in loop) ==========
void handlePlayback() {
  if (!playing || playSeq == nullptr || playLen == 0) return;

  unsigned long now = millis();
  Pose &cur = playSeq[playIndex];

  // Apply current frame once when index changes
  if (playIndex != lastFrameIndex) {
    for (int i=0;i<NUM_ARM_SERVOS;i++)
      setServo(i, cur.servo[i]);   // 0..5 arm only
    setMotors(cur.leftSpeed, cur.rightSpeed);
    frameStartTime = now;
    lastFrameIndex = playIndex;
  }

  if (now - frameStartTime >= cur.durationMs) {
    playIndex++;
    if (playIndex >= playLen) {
      playing        = false;
      playSeq        = nullptr;
      playLen        = 0;
      playIndex      = 0;
      lastFrameIndex = -1;
      stopMotors();
      currentMode = MODE_IDLE;
      updateLEDs();
    }
  }
}

// ========== Continuous wiper update (non-blocking) ==========
void updateWiper() {
  // Only run in cleaning mode & not obstacle stop
  if (currentMode != MODE_CLEANING) return;

  unsigned long now = millis();
  if (now - lastWiperUpdate < WIPER_STEP_MS) return;
  lastWiperUpdate = now;

  wiperAngle += wiperDir * WIPER_STEP_DEG;

  if (wiperAngle >= WIPER_MAX_ANGLE) {
    wiperAngle = WIPER_MAX_ANGLE;
    wiperDir   = -1;
  } else if (wiperAngle <= WIPER_MIN_ANGLE) {
    wiperAngle = WIPER_MIN_ANGLE;
    wiperDir   = +1;
  }

  setServo(SERVO_WIPER, wiperAngle);
}

// ========== WiFi Handlers ==========
// 1) Manual drive: /drive?cmd=F/B/L/R/S
void handleDrive() {
  if (currentMode == MODE_OBSTACLE_STOP) {
    server.send(200, "text/plain", "Obstacle - drive blocked");
    return;
  }

  String cmd = server.hasArg("cmd") ? server.arg("cmd") : "";
  if (cmd == "F") {
    setMotors(200, 200);
  } else if (cmd == "B") {
    setMotors(-200, -200);
  } else if (cmd == "L") {
    setMotors(-150, 150);
  } else if (cmd == "R") {
    setMotors(150, -150);
  } else {
    setMotors(0, 0);
  }
  server.send(200, "text/plain", "OK drive " + cmd);
}

// 2) Servo control: /servo?ch=0-6&ang=0-180
void handleServo() {
  int ch  = server.hasArg("ch") ? server.arg("ch").toInt() : -1;
  int ang = server.hasArg("ang") ? server.arg("ang").toInt() : 90;
  if (ch < 0 || ch >= NUM_SERVOS) {
    server.send(400, "text/plain", "bad channel");
    return;
  }
  if (ang < 0) ang = 0;
  if (ang > 180) ang = 180;
  setServo(ch, ang);
  server.send(200, "text/plain", "OK servo");
}

// 3) Save pose: /save?mode=water&dur=2000
void handleSave() {
  String mode = server.hasArg("mode") ? server.arg("mode") : "";
  int dur = server.hasArg("dur") ? server.arg("dur").toInt() : 1500;

  bool ok = false;
  if (mode == "water") {
    ok = savePoseToSeq(waterSeq, waterLen, dur);
  } else if (mode == "med") {
    ok = savePoseToSeq(medSeq, medLen, dur);
  } else if (mode == "garbage") {
    ok = savePoseToSeq(garbageSeq, garbageLen, dur);
  } else if (mode == "clean") {
    ok = savePoseToSeq(cleanSeq, cleanLen, dur);
  }

  if (ok) server.send(200, "text/plain", "Saved frame");
  else    server.send(500, "text/plain", "Seq full or bad mode");
}

// 4) Play mode once: /play?mode=water
void handlePlay() {
  String mode = server.hasArg("mode") ? server.arg("mode") : "";
  if (mode == "water") {
    startPlay(waterSeq, waterLen, MODE_WATER);
  } else if (mode == "med") {
    startPlay(medSeq, medLen, MODE_MEDICINE);
  } else if (mode == "garbage") {
    startPlay(garbageSeq, garbageLen, MODE_GARBAGE);
  } else if (mode == "clean") {
    startPlay(cleanSeq, cleanLen, MODE_CLEANING);
  }
  server.send(200, "text/plain", "Play triggered");
}

// Root: help text
void handleRoot() {
  String msg = "HEBA Robot API:\n";
  msg += "/drive?cmd=F/B/L/R/S\n";
  msg += "/servo?ch=0-6&ang=0-180\n";
  msg += "/save?mode=water|med|garbage|clean&dur=ms\n";
  msg += "/play?mode=water|med|garbage|clean\n";
  server.send(200, "text/plain", msg);
}

// ========== Obstacle logic ==========
void checkObstacle() {
  long d = getDistanceCm();
  if (d < OBSTACLE_CM) {
    if (currentMode != MODE_OBSTACLE_STOP) {
      prevMode = currentMode;
      currentMode = MODE_OBSTACLE_STOP;
      stopMotors();
      updateLEDs();
      lcd.clear();
      lcd.setCursor(0,0); lcd.print("Obstacle!");
      lcd.setCursor(0,1); lcd.print("Dist: "); lcd.print(d); lcd.print("cm");
    }
  } else {
    if (currentMode == MODE_OBSTACLE_STOP) {
      currentMode = prevMode;
      updateLEDs();
      updateLCD();
    }
  }
}

// ========== RTC Schedules ==========
void handleSchedule() {
  DateTime now = rtc.now();
  int minute = now.minute();

  if (minute != lastScheduleMinute) {
    lastScheduleMinute = minute;

    int m = minute % 4;

    if (!playing && currentMode != MODE_OBSTACLE_STOP) {
      if (m == 0 && cleanLen > 0) {
        startPlay(cleanSeq, cleanLen, MODE_CLEANING);
      } else if (m == 1 && waterLen > 0) {
        startPlay(waterSeq, waterLen, MODE_WATER);
      } else if (m == 2 && medLen > 0) {
        startPlay(medSeq, medLen, MODE_MEDICINE);
      } else if (m == 3 && garbageLen > 0) {
        startPlay(garbageSeq, garbageLen, MODE_GARBAGE);
      }
    }
  }
}

// ========== Setup ==========
void setup() {
  Serial.begin(115200);

  // Pins
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  // PWM channels for ENA/ENB
  ledcSetup(0, 1000, 8); // channel 0, 1kHz, 8-bit
  ledcAttachPin(ENA, 0);
  ledcSetup(1, 1000, 8);
  ledcAttachPin(ENB, 1);

  stopMotors();

  // I2C
  Wire.begin(21, 22);

  // PCA9685
  pca.begin();
  pca.setPWMFreq(50);  // servos ~50Hz
  delay(10);

  // RTC
  if (!rtc.begin()) {
    Serial.println("RTC not found!");
  }
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // LCD
  lcd.init();
  lcd.backlight();
  showStatus("HEBA Booting...", "Please wait");

  // WiFi AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // Web routes
  server.on("/", handleRoot);
  server.on("/drive", handleDrive);
  server.on("/servo", handleServo);
  server.on("/save", handleSave);
  server.on("/play", handlePlay);
  server.begin();
  Serial.println("HTTP server started");

  // Start position (all servos 90 deg)
  for (int i=0;i<NUM_SERVOS;i++) setServo(i, 90);

  // Wiper start at 0°
  wiperAngle = WIPER_MIN_ANGLE;
  setServo(SERVO_WIPER, wiperAngle);

  // Hardcoded demo cleaning sequence ready
  initDemoCleaningSequence();

  currentMode = MODE_IDLE;
  updateLEDs();
  updateLCD();
}

// ========== Loop ==========
unsigned long lastLCDupdate = 0;

void loop() {
  server.handleClient();   // WiFi commands

  checkObstacle();         // obstacle logic
  handleSchedule();        // RTC-based schedules
  handlePlayback();        // play taught sequences
  updateWiper();           // continuous wiper (for cleaning mode)

  // LCD refresh every ~1s when not obstacle
  if (currentMode != MODE_OBSTACLE_STOP) {
    unsigned long now = millis();
    if (now - lastLCDupdate > 1000) {
      updateLCD();
      lastLCDupdate = now;
    }
  }
}
