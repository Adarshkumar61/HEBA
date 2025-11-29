#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <WiFi.h>
#include <EEPROM.h>

// WiFi Credentials for RoboRemo
const char* ssid = "RobotTeach";
const char* password = "teach1234";
WiFiServer server(80);

// Hardware Objects
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;

// Pin Definitions
#define TRIG_PIN 5
#define ECHO_PIN 18
#define LED_GREEN 25
#define LED_RED 33
#define LED_YELLOW 32
#define MOTOR_IN1 26
#define MOTOR_IN2 27
#define MOTOR_IN3 14
#define MOTOR_IN4 12

// Servo Channels on PCA9685 (6 servos for arm + 1 wiper)
#define WIPER_SERVO 0      // Front wiper
#define ARM_BASE 1         // Base rotation
#define ARM_SHOULDER 2     // Shoulder joint
#define ARM_ELBOW 3        // Elbow joint
#define ARM_WRIST_ROTATE 4 // Wrist rotation
#define ARM_WRIST_PITCH 5  // Wrist pitch
#define ARM_GRIPPER 6      // Gripper

// Servo Limits
#define SERVO_MIN 150
#define SERVO_MAX 600

// Teaching Storage
#define MAX_STEPS 50
#define EEPROM_SIZE 4096

struct TeachStep {
  int servoPos[7];  // 7 servos (1 wiper + 6 arm)
  int motorL;       // -255 to 255
  int motorR;
  int duration;     // ms
};

struct TeachSequence {
  TeachStep steps[MAX_STEPS];
  int stepCount;
};

TeachSequence sequences[4]; // 4 modes: Water, Medicine, Garbage, Cleaning
int currentMode = -1;
bool isTeaching = false;
bool isPlaying = false;
int teachIndex = 0;

// Current positions (wiper + 6 arm servos)
int servoPositions[7] = {450, 300, 300, 300, 300, 300, 300}; // Wiper up, arm center
int motorSpeed = 0;

// Mode Names
const char* modeNames[] = {"Water", "Medicine", "Garbage", "Cleaning"};

// RTC Schedule (hour, minute, mode)
struct Schedule {
  int hour;
  int minute;
  int mode;
};

Schedule schedules[] = {
  {8, 0, 3},   // 8:00 AM - Cleaning
  {8, 1, 2},   // 8:01 AM - Garbage
  {8, 2, 0},   // 8:02 AM - Water
  {8, 3, 1},   // 8:03 AM - Medicine
  {12, 0, 3},  // 12:00 PM - Cleaning
  {12, 1, 0},  // 12:01 PM - Water
  {18, 0, 3},  // 6:00 PM - Cleaning
  {18, 1, 1}   // 6:01 PM - Medicine
};
int scheduleCount = 8;

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C
  Wire.begin(21, 22);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Robot Starting..");
  
  // Initialize RTC
  if (!rtc.begin()) {
    lcd.clear();
    lcd.print("RTC Error!");
    while(1);
  }
  
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  // Initialize PWM Servo Driver
  pwm.begin();
  pwm.setPWMFreq(60);
  
  // Initialize Pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(MOTOR_IN3, OUTPUT);
  pinMode(MOTOR_IN4, OUTPUT);
  
  // Set default LED (Yellow - Idle)
  setLED('Y');
  
  // Move all servos to default position
  for(int i = 0; i < 7; i++) {
    pwm.setPWM(i, 0, servoPositions[i]);
  }
  
  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  loadSequences();
  
  // Start WiFi AP
  WiFi.softAP(ssid, password);
  server.begin();
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi: ");
  lcd.print(ssid);
  lcd.setCursor(0, 1);
  lcd.print(WiFi.softAPIP());
  
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ready! 6-Servo");
  
  Serial.println("Robot Ready! (6 Servo Arm)");
  Serial.println("IP: " + WiFi.softAPIP().toString());
}

void loop() {
  // Check RTC for scheduled tasks
  checkSchedule();
  
  // Check obstacle
  if (checkObstacle()) {
    stopMotors();
    setLED('R');
    lcd.setCursor(0, 1);
    lcd.print("OBSTACLE!       ");
    delay(500);
    return;
  }
  
  // Handle WiFi commands
  handleWiFi();
  
  // If playing sequence
  if (isPlaying && currentMode >= 0) {
    playSequence(currentMode);
  } else if (!isTeaching && !isPlaying) {
    setLED('Y'); // Idle
    lcd.setCursor(0, 1);
    lcd.print("Idle            ");
  }
  
  delay(50);
}

void handleWiFi() {
  WiFiClient client = server.available();
  if (!client) return;
  
  String cmd = "";
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      if (c == '\n') {
        processCommand(cmd);
        cmd = "";
      } else {
        cmd += c;
      }
    }
  }
  client.stop();
}

void processCommand(String cmd) {
  cmd.trim();
  Serial.println("CMD: " + cmd);
  
  // Teaching commands
  if (cmd.startsWith("TEACH_START:")) {
    int mode = cmd.substring(12).toInt();
    startTeaching(mode);
  }
  else if (cmd == "TEACH_STEP") {
    recordTeachStep();
  }
  else if (cmd == "TEACH_END") {
    endTeaching();
  }
  else if (cmd.startsWith("PLAY:")) {
    int mode = cmd.substring(5).toInt();
    startPlaying(mode);
  }
  else if (cmd == "STOP") {
    stopAll();
  }
  
  // Manual servo control during teaching (S0-S6)
  else if (cmd.startsWith("S0:")) servoPositions[0] = cmd.substring(3).toInt(); // Wiper
  else if (cmd.startsWith("S1:")) servoPositions[1] = cmd.substring(3).toInt(); // Base
  else if (cmd.startsWith("S2:")) servoPositions[2] = cmd.substring(3).toInt(); // Shoulder
  else if (cmd.startsWith("S3:")) servoPositions[3] = cmd.substring(3).toInt(); // Elbow
  else if (cmd.startsWith("S4:")) servoPositions[4] = cmd.substring(3).toInt(); // Wrist Rotate
  else if (cmd.startsWith("S5:")) servoPositions[5] = cmd.substring(3).toInt(); // Wrist Pitch
  else if (cmd.startsWith("S6:")) servoPositions[6] = cmd.substring(3).toInt(); // Gripper
  
  // Motor control
  else if (cmd == "FWD") moveMotors(200, 200);
  else if (cmd == "BWD") moveMotors(-200, -200);
  else if (cmd == "LEFT") moveMotors(-150, 150);
  else if (cmd == "RIGHT") moveMotors(150, -150);
  else if (cmd == "STOP_M") stopMotors();
  
  // Update all servos
  for(int i = 0; i < 7; i++) {
    pwm.setPWM(i, 0, servoPositions[i]);
  }
}

void startTeaching(int mode) {
  if (mode < 0 || mode > 3) return;
  
  isTeaching = true;
  isPlaying = false;
  currentMode = mode;
  teachIndex = 0;
  sequences[mode].stepCount = 0;
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Teach: ");
  lcd.print(modeNames[mode]);
  lcd.setCursor(0, 1);
  lcd.print("Step: 0         ");
  
  Serial.println("Teaching mode: " + String(modeNames[mode]));
}

void recordTeachStep() {
  if (!isTeaching || teachIndex >= MAX_STEPS) return;
  
  TeachStep step;
  // Save all 7 servo positions
  for(int i = 0; i < 7; i++) {
    step.servoPos[i] = servoPositions[i];
  }
  step.motorL = motorSpeed;
  step.motorR = motorSpeed;
  step.duration = 1000; // 1 second per step
  
  sequences[currentMode].steps[teachIndex] = step;
  teachIndex++;
  sequences[currentMode].stepCount = teachIndex;
  
  lcd.setCursor(0, 1);
  lcd.print("Step: ");
  lcd.print(teachIndex);
  lcd.print("        ");
  
  Serial.println("Recorded step: " + String(teachIndex));
  Serial.print("Servos: ");
  for(int i = 0; i < 7; i++) {
    Serial.print(servoPositions[i]);
    Serial.print(" ");
  }
  Serial.println();
}

void endTeaching() {
  if (!isTeaching) return;
  
  isTeaching = false;
  saveSequences();
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Saved: ");
  lcd.print(modeNames[currentMode]);
  lcd.setCursor(0, 1);
  lcd.print("Steps: ");
  lcd.print(sequences[currentMode].stepCount);
  
  delay(2000);
  currentMode = -1;
  
  Serial.println("Teaching ended and saved");
}

void startPlaying(int mode) {
  if (mode < 0 || mode > 3 || sequences[mode].stepCount == 0) return;
  
  isPlaying = true;
  isTeaching = false;
  currentMode = mode;
  teachIndex = 0;
  
  setLED('G');
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Mode: ");
  lcd.print(modeNames[mode]);
  
  Serial.println("Playing: " + String(modeNames[mode]));
}

void playSequence(int mode) {
  if (teachIndex >= sequences[mode].stepCount) {
    // Sequence complete
    stopAll();
    lcd.setCursor(0, 1);
    lcd.print("Complete!       ");
    delay(2000);
    isPlaying = false;
    currentMode = -1;
    return;
  }
  
  TeachStep step = sequences[mode].steps[teachIndex];
  
  // Move all 7 servos
  for(int i = 0; i < 7; i++) {
    pwm.setPWM(i, 0, step.servoPos[i]);
  }
  
  // Move motors
  moveMotors(step.motorL, step.motorR);
  
  lcd.setCursor(0, 1);
  lcd.print("Step: ");
  lcd.print(teachIndex + 1);
  lcd.print("/");
  lcd.print(sequences[mode].stepCount);
  
  delay(step.duration);
  teachIndex++;
}

void checkSchedule() {
  DateTime now = rtc.now();
  
  for(int i = 0; i < scheduleCount; i++) {
    if (now.hour() == schedules[i].hour && 
        now.minute() == schedules[i].minute &&
        now.second() == 0) {
      
      // Special handling for cleaning mode
      if (schedules[i].mode == 3) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Cleaning Mode");
        
        // Lower wiper
        lcd.setCursor(0, 1);
        lcd.print("Wiper Down...");
        pwm.setPWM(WIPER_SERVO, 0, 150);
        delay(1000);
        
        // Start cleaning sequence
        startPlaying(schedules[i].mode);
        
        // Wait for sequence to complete
        while(isPlaying) {
          loop();
        }
        
        // Raise wiper back up
        lcd.setCursor(0, 1);
        lcd.print("Wiper Up...");
        pwm.setPWM(WIPER_SERVO, 0, 450);
        delay(1000);
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Cleaning Done!");
        delay(2000);
      } else {
        // Normal modes (Water, Medicine, Garbage)
        startPlaying(schedules[i].mode);
      }
      
      delay(1000); // Prevent multiple triggers
    }
  }
}

bool checkObstacle() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  int distance = duration * 0.034 / 2;
  
  return (distance > 0 && distance < 20); // 20cm threshold
}

void moveMotors(int left, int right) {
  motorSpeed = (left + right) / 2;
  
  // Left motor
  if (left > 0) {
    digitalWrite(MOTOR_IN1, HIGH);
    digitalWrite(MOTOR_IN2, LOW);
  } else if (left < 0) {
    digitalWrite(MOTOR_IN1, LOW);
    digitalWrite(MOTOR_IN2, HIGH);
  } else {
    digitalWrite(MOTOR_IN1, LOW);
    digitalWrite(MOTOR_IN2, LOW);
  }
  
  // Right motor
  if (right > 0) {
    digitalWrite(MOTOR_IN3, HIGH);
    digitalWrite(MOTOR_IN4, LOW);
  } else if (right < 0) {
    digitalWrite(MOTOR_IN3, LOW);
    digitalWrite(MOTOR_IN4, HIGH);
  } else {
    digitalWrite(MOTOR_IN3, LOW);
    digitalWrite(MOTOR_IN4, LOW);
  }
}

void stopMotors() {
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, LOW);
  digitalWrite(MOTOR_IN4, LOW);
  motorSpeed = 0;
}

void stopAll() {
  stopMotors();
  isPlaying = false;
  isTeaching = false;
}

void setLED(char color) {
  digitalWrite(LED_GREEN, color == 'G' ? HIGH : LOW);
  digitalWrite(LED_RED, color == 'R' ? HIGH : LOW);
  digitalWrite(LED_YELLOW, color == 'Y' ? HIGH : LOW);
}

void saveSequences() {
  int addr = 0;
  for(int i = 0; i < 4; i++) {
    EEPROM.put(addr, sequences[i]);
    addr += sizeof(TeachSequence);
  }
  EEPROM.commit();
  Serial.println("Sequences saved to EEPROM");
}

void loadSequences() {
  int addr = 0;
  for(int i = 0; i < 4; i++) {
    EEPROM.get(addr, sequences[i]);
    addr += sizeof(TeachSequence);
  }
  Serial.println("Sequences loaded from EEPROM");
}