#include <Wire.h>
#include <Servo.h>
#include <SoftwareSerial.h>

// =================================================
// MOTOR (L298N)
// =================================================
const int IN1 = 7;
const int IN2 = 4;
const int IN3 = 8;
const int IN4 = 12;
const int ENA = 5;
const int ENB = 6;

// =================================================
// SERVOS
// =================================================
Servo servoRight;
Servo servoLeft;
Servo servoHead;

const int SERVO_RIGHT_PIN = 9;
const int SERVO_LEFT_PIN  = 10;
const int SERVO_HEAD_PIN  = 11;

const int HEAD_CENTER = 90;
const int HEAD_UP     = 120;
const int HEAD_DOWN   = 60;

// =================================================
// SOFTWARE SERIAL (ESP32 → ARDUINO)
// =================================================
SoftwareSerial espSerial(2, -1); // RX=D2, TX unused

// =================================================
// FSM ENUMS
// =================================================
enum ActState {
  ACT_IDLE,
  MOVE_FWD, MOVE_BWD, TURN_L, TURN_R,
  LOOK_UP, LOOK_DOWN, CENTER_HEAD,
  NOD_HEAD, SHAKE_HEAD,
  WAVE_RH, WAVE_LH,
  ATTEND_USER,
  AVOID_OBS
};

ActState currentAct = ACT_IDLE;
ActState nextAct    = ACT_IDLE;

// =================================================
// SERIAL BUFFERS
// =================================================
String usbBuffer = "";
String espBuffer = "";

// =================================================
// SETUP
// =================================================
void setup() {
  Serial.begin(9600);      // USB DEBUG
  espSerial.begin(9600);  // ESP32 UART

  Serial.println("\n[BOOT] Arduino FSM + SoftwareSerial READY");

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);

  servoRight.attach(SERVO_RIGHT_PIN);
  servoLeft.attach(SERVO_LEFT_PIN);
  servoHead.attach(SERVO_HEAD_PIN);

  servoHead.write(HEAD_CENTER);
  servoRight.write(90);
  servoLeft.write(90);

  stopMotors();
}

// =================================================
// LOOP
// =================================================
void loop() {
  readUsbSerial();   // existing debug path
  readEspSerial();   // NEW ESP32 path

  if (nextAct != currentAct) {
    Serial.print("[FSM] Transition ");
    Serial.print(currentAct);
    Serial.print(" → ");
    Serial.println(nextAct);

    currentAct = nextAct;
    executeAct(currentAct);
  }
}

// =================================================
// USB SERIAL (UNCHANGED LOGIC)
// =================================================
void readUsbSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      usbBuffer.trim();
      Serial.print("[USB RX] ");
      Serial.println(usbBuffer);
      parseCommand(usbBuffer);
      usbBuffer = "";
    } else {
      usbBuffer += c;
    }
  }
}

// =================================================
// ESP32 SOFTWARE SERIAL (NEW)
// =================================================
void readEspSerial() {
  while (espSerial.available()) {
    char c = espSerial.read();
    Serial.print("[ESP RX CHAR] ");
    Serial.println(c);

    if (c == '\n') {
      espBuffer.trim();
      Serial.print("[ESP RX CMD] ");
      Serial.println(espBuffer);
      parseCommand(espBuffer);
      espBuffer = "";
    } else {
      espBuffer += c;
    }
  }
}

// =================================================
// PARSER (UNCHANGED)
// =================================================
void parseCommand(String cmd) {
  if (!cmd.startsWith("ACT:")) {
    Serial.println("[PARSE] Ignored");
    return;
  }

  String act = cmd.substring(4);
  Serial.print("[PARSE] ACT = ");
  Serial.println(act);

  if      (act == "MOVE_FORWARD")   nextAct = MOVE_FWD;
  else if (act == "MOVE_BACKWARD")  nextAct = MOVE_BWD;
  else if (act == "TURN_LEFT")      nextAct = TURN_L;
  else if (act == "TURN_RIGHT")     nextAct = TURN_R;
  else if (act == "STOP_ALL")       nextAct = ACT_IDLE;
  else if (act == "LOOK_UP")        nextAct = LOOK_UP;
  else if (act == "LOOK_DOWN")      nextAct = LOOK_DOWN;
  else if (act == "CENTER_HEAD")    nextAct = CENTER_HEAD;
  else if (act == "NOD_HEAD")       nextAct = NOD_HEAD;
  else if (act == "WAVE_RIGHT_HAND")nextAct = WAVE_RH;
  else if (act == "WAVE_LEFT_HAND") nextAct = WAVE_LH;
  else if (act == "ATTEND_USER")    nextAct = ATTEND_USER;
  else                              nextAct = ACT_IDLE;
}

// =================================================
// ACT EXECUTION (UNCHANGED)
// =================================================
void executeAct(ActState act) {
  stopMotors();

  switch (act) {
    case MOVE_FWD: motorsForward(); break;
    case MOVE_BWD: motorsBackward(); break;
    case TURN_L: motorsLeft(); break;
    case TURN_R: motorsRight(); break;

    case LOOK_UP: servoHead.write(HEAD_UP); break;
    case LOOK_DOWN: servoHead.write(HEAD_DOWN); break;
    case CENTER_HEAD: servoHead.write(HEAD_CENTER); break;

    case WAVE_RH: waveServo(servoRight); break;
    case WAVE_LH: waveServo(servoLeft); break;

    case ATTEND_USER:
      servoHead.write(HEAD_CENTER);
      servoRight.write(90);
      servoLeft.write(90);
      break;

    default: break;
  }
}

// =================================================
// LOW LEVEL (UNCHANGED)
// =================================================
void motorsForward() {
  analogWrite(ENA, 150); analogWrite(ENB, 150);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void motorsBackward() {
  analogWrite(ENA, 150); analogWrite(ENB, 150);
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
}

void motorsLeft() {
  analogWrite(ENA, 120); analogWrite(ENB, 120);
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void motorsRight() {
  analogWrite(ENA, 120); analogWrite(ENB, 120);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
}

void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

void waveServo(Servo &s) {
  for (int i = 0; i < 3; i++) {
    s.write(60); delay(200);
    s.write(120); delay(200);
  }
  s.write(90);
}
