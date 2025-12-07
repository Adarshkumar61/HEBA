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




// for arm only :

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Preferences.h>

// WiFi credentials
const char* ssid = "RoboArm_5DOF";
const char* password = "12345678";

// PCA9685
Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(0x40);
Preferences preferences;

WebServer server(80);

// SERVO CONFIGURATION
// Channels 0,1,2 = MG996R (bigger servos)
// Channels 3,4,5 = SG90 (smaller servos)

// MG996R pulse values (CHANGE TO YOUR WORKING SET!)
// If SET A worked: 150, 300, 450
// If SET B worked: 205, 307, 410
// If SET C worked: 175, 375, 575
#define MG_MIN 205
#define MG_MID 307
#define MG_MAX 410

// SG90 pulse values (standard)
#define SG_MIN 150
#define SG_MID 300
#define SG_MAX 450

// Servo structure
struct Servo {
  int channel;
  int angle;
  int minPulse;
  int maxPulse;
  String name;
};

Servo servos[6] = {
  {0, 90, MG_MIN, MG_MAX, "Base"},      // MG996R
  {1, 90, MG_MIN, MG_MAX, "Shoulder"},  // MG996R
  {2, 90, MG_MIN, MG_MAX, "Elbow"},     // MG996R
  {3, 90, SG_MIN, SG_MAX, "Wrist"},     // SG90
  {4, 90, SG_MIN, SG_MAX, "Rotate"},    // SG90
  {5, 90, SG_MIN, SG_MAX, "Gripper"}    // SG90
};

// Training mode
#define MAX_POSITIONS 50
struct Position {
  int angles[6];
  int delayTime;
};

Position trainedSequence[MAX_POSITIONS];
int sequenceLength = 0;
bool isTraining = false;
bool isPlaying = false;

int angleToPulse(int angle, int minPulse, int maxPulse) {
  angle = constrain(angle, 0, 180);
  return map(angle, 0, 180, minPulse, maxPulse);
}

void moveServo(int index, int angle) {
  servos[index].angle = constrain(angle, 0, 180);
  int pulse = angleToPulse(servos[index].angle, servos[index].minPulse, servos[index].maxPulse);
  pca.setPWM(servos[index].channel, 0, pulse);
}

void moveAllToHome() {
  for(int i = 0; i < 6; i++) {
    moveServo(i, 90);
  }
}

void saveSequence() {
  preferences.begin("robotarm", false);
  preferences.putInt("seqLen", sequenceLength);
  
  for(int i = 0; i < sequenceLength; i++) {
    String key = "pos" + String(i);
    uint8_t data[28]; // 6 angles * 4 bytes + 1 delay * 4 bytes
    
    for(int j = 0; j < 6; j++) {
      memcpy(&data[j*4], &trainedSequence[i].angles[j], 4);
    }
    memcpy(&data[24], &trainedSequence[i].delayTime, 4);
    
    preferences.putBytes(key.c_str(), data, 28);
  }
  
  preferences.end();
  Serial.println("✅ Sequence saved!");
}

void loadSequence() {
  preferences.begin("robotarm", true);
  sequenceLength = preferences.getInt("seqLen", 0);
  
  for(int i = 0; i < sequenceLength; i++) {
    String key = "pos" + String(i);
    uint8_t data[28];
    preferences.getBytes(key.c_str(), data, 28);
    
    for(int j = 0; j < 6; j++) {
      memcpy(&trainedSequence[i].angles[j], &data[j*4], 4);
    }
    memcpy(&trainedSequence[i].delayTime, &data[24], 4);
  }
  
  preferences.end();
  Serial.println("✅ Sequence loaded: " + String(sequenceLength) + " positions");
}

void playSequence() {
  if(sequenceLength == 0) return;
  
  isPlaying = true;
  Serial.println("▶️ Playing sequence...");
  
  for(int i = 0; i < sequenceLength && isPlaying; i++) {
    // Move all servos to position
    for(int j = 0; j < 6; j++) {
      moveServo(j, trainedSequence[i].angles[j]);
    }
    
    Serial.print("Position ");
    Serial.print(i + 1);
    Serial.print("/");
    Serial.println(sequenceLength);
    
    delay(trainedSequence[i].delayTime);
  }
  
  isPlaying = false;
  Serial.println("✅ Playback complete!");
}

String getHTML() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body{font-family:Arial;margin:0;padding:20px;background:#0a0a0a;color:#fff}";
  html += ".container{max-width:800px;margin:0 auto}";
  html += "h1{text-align:center;color:#00ff88;text-shadow:0 0 10px #00ff88}";
  html += ".mode{display:flex;gap:10px;margin:20px 0}";
  html += ".mode button{flex:1;padding:15px;font-size:18px;border:none;cursor:pointer;border-radius:8px}";
  html += ".active{background:#00ff88;color:#000}";
  html += ".inactive{background:#333;color:#fff}";
  html += ".servo-control{background:#1a1a1a;padding:15px;margin:10px 0;border-radius:8px;border:1px solid #333}";
  html += ".servo-name{color:#00ff88;font-size:20px;margin-bottom:10px}";
  html += ".servo-value{color:#fff;font-size:24px;text-align:center;margin:10px 0}";
  html += "input[type=range]{width:100%;height:40px;margin:10px 0}";
  html += ".controls{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin:20px 0}";
  html += "button{background:#00ff88;border:none;color:#000;padding:15px;font-size:16px;";
  html += "cursor:pointer;border-radius:8px;font-weight:bold}";
  html += "button:hover{background:#00cc70}";
  html += ".train-btn{background:#ff9500}";
  html += ".train-btn:hover{background:#cc7700}";
  html += ".danger{background:#ff3b30}";
  html += ".danger:hover{background:#cc2f26}";
  html += ".info{background:#1a1a1a;padding:15px;margin:20px 0;border-radius:8px;border:1px solid #00ff88}";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>🦾 5-DOF ROBOTIC ARM</h1>";
  
  // Mode selection
  html += "<div class='mode'>";
  html += "<button class='" + String(isTraining ? "active" : "inactive") + "' onclick='setMode(\"train\")'>📝 TRAIN</button>";
  html += "<button class='" + String(!isTraining ? "active" : "inactive") + "' onclick='setMode(\"control\")'>🎮 CONTROL</button>";
  html += "</div>";
  
  // Info panel
  html += "<div class='info'>";
  html += "<strong>Saved Positions:</strong> " + String(sequenceLength) + "/" + String(MAX_POSITIONS);
  html += "<br><strong>Status:</strong> " + String(isPlaying ? "Playing ▶️" : isTraining ? "Training 📝" : "Ready ✓");
  html += "</div>";
  
  // Servo controls
  for(int i = 0; i < 6; i++) {
    html += "<div class='servo-control'>";
    html += "<div class='servo-name'>" + servos[i].name + "</div>";
    html += "<div class='servo-value' id='val" + String(i) + "'>" + String(servos[i].angle) + "°</div>";
    html += "<input type='range' min='0' max='180' value='" + String(servos[i].angle) + "' ";
    html += "oninput='updateServo(" + String(i) + ",this.value)'>";
    html += "</div>";
  }
  
  // Control buttons
  html += "<div class='controls'>";
  html += "<button onclick='home()'>🏠 HOME</button>";
  html += "<button onclick='stopAll()'>⛔ STOP</button>";
  html += "<button class='train-btn' onclick='capturePosition()'>📸 CAPTURE</button>";
  html += "<button onclick='playSequence()'>▶️ PLAY</button>";
  html += "<button onclick='saveSequence()'>💾 SAVE</button>";
  html += "<button onclick='loadSequence()'>📂 LOAD</button>";
  html += "<button class='danger' onclick='clearSequence()'>🗑️ CLEAR</button>";
  html += "</div>";
  
  html += "</div>";
  
  html += "<script>";
  html += "function updateServo(idx,val){";
  html += "document.getElementById('val'+idx).innerText=val+'°';";
  html += "fetch('/servo?idx='+idx+'&angle='+val);}";
  html += "function setMode(m){fetch('/mode?m='+m).then(()=>location.reload());}";
  html += "function home(){fetch('/home').then(()=>location.reload());}";
  html += "function stopAll(){fetch('/stop').then(()=>location.reload());}";
  html += "function capturePosition(){fetch('/capture').then(r=>r.text()).then(t=>alert(t));}";
  html += "function playSequence(){if(confirm('Play sequence?')){fetch('/play');alert('Playing...');}}";
  html += "function saveSequence(){fetch('/save').then(()=>alert('Saved!'));}";
  html += "function loadSequence(){fetch('/load').then(()=>location.reload());}";
  html += "function clearSequence(){if(confirm('Clear all positions?')){fetch('/clear').then(()=>location.reload());}}";
  html += "</script></body></html>";
  
  return html;
}

void handleRoot() {
  server.send(200, "text/html", getHTML());
}

void handleServo() {
  if(server.hasArg("idx") && server.hasArg("angle")) {
    int idx = server.arg("idx").toInt();
    int angle = server.arg("angle").toInt();
    moveServo(idx, angle);
    server.send(200, "text/plain", "OK");
  }
}

void handleMode() {
  if(server.hasArg("m")) {
    String mode = server.arg("m");
    isTraining = (mode == "train");
    server.send(200, "text/plain", "OK");
  }
}

void handleHome() {
  moveAllToHome();
  server.send(200, "text/plain", "OK");
}

void handleStop() {
  isPlaying = false;
  for(int i = 0; i < 16; i++) {
    pca.setPWM(i, 0, 0);
  }
  server.send(200, "text/plain", "OK");
}

void handleCapture() {
  if(sequenceLength >= MAX_POSITIONS) {
    server.send(400, "text/plain", "Sequence full!");
    return;
  }
  
  for(int i = 0; i < 6; i++) {
    trainedSequence[sequenceLength].angles[i] = servos[i].angle;
  }
  trainedSequence[sequenceLength].delayTime = 1000; // 1 sec default
  
  sequenceLength++;
  server.send(200, "text/plain", "Position " + String(sequenceLength) + " captured!");
}

void handlePlay() {
  playSequence();
  server.send(200, "text/plain", "OK");
}

void handleSave() {
  saveSequence();
  server.send(200, "text/plain", "OK");
}

void handleLoad() {
  loadSequence();
  server.send(200, "text/plain", "OK");
}

void handleClear() {
  sequenceLength = 0;
  preferences.begin("robotarm", false);
  preferences.clear();
  preferences.end();
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n╔════════════════════════════════════╗");
  Serial.println("║   5-DOF ROBOTIC ARM CONTROL       ║");
  Serial.println("╚════════════════════════════════════╝\n");
  
  // Initialize I2C
  Wire.begin(21, 22);
  
  // Initialize PCA9685
  pca.begin();
  pca.setPWMFreq(50);
  delay(100);
  
  // Turn off all channels
  for(int i = 0; i < 16; i++) {
    pca.setPWM(i, 0, 0);
  }
  
  // Move to home position
  moveAllToHome();
  Serial.println("✅ All servos at home (90°)");
  
  // Load saved sequence
  loadSequence();
  
  // Start WiFi AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  
  IPAddress IP = WiFi.softAPIP();
  
  Serial.println("\n╔════════════════════════════════════╗");
  Serial.println("║        WiFi AP Started            ║");
  Serial.println("╠════════════════════════════════════╣");
  Serial.print("║ SSID: ");
  Serial.println(ssid);
  Serial.print("║ Pass: ");
  Serial.println(password);
  Serial.print("║ IP:   ");
  Serial.println(IP);
  Serial.println("╚════════════════════════════════════╝\n");
  
  // Setup routes
  server.on("/", handleRoot);
  server.on("/servo", handleServo);
  server.on("/mode", handleMode);
  server.on("/home", handleHome);
  server.on("/stop", handleStop);
  server.on("/capture", handleCapture);
  server.on("/play", handlePlay);
  server.on("/save", handleSave);
  server.on("/load", handleLoad);
  server.on("/clear", handleClear);
  
  server.begin();
  Serial.println("🌐 Web server started!");
  Serial.println("📱 Connect to: RoboArm_5DOF");
  Serial.println("🌍 Open: http://192.168.4.1\n");
}

void loop() {
  server.handleClient();
}