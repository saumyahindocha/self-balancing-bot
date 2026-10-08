/*
 * NIDS Two-Wheeled Self-Balancing Robot - full receiver (ARCHIVED)
 * firmware/archive/receiver_nrf_ir/receiver_nrf_ir.ino
 *
 * STATUS: shelved on 18 Aug 2026. It compiled cleanly (21 Jun 2026) but was
 * never run on the assembled robot. The current build uses firmware/balance.
 *
 * What it does: Kalman-filtered pitch -> PID balance, plus a direction bias on
 * top of the balance output that comes from either
 *   - IR line following (3 digital IR sensors), or
 *   - manual joystick control received over NRF24L01 from the remote.
 * A press of the remote's button/switch toggles IR <-> MANUAL. If no packet
 * arrives for 500 ms the robot falls back to IR mode.
 *
 * Board     : Arduino Mega 2560
 * Libraries : FastIMU (LiquidCGS), RF24 (TMRh20)
 *
 * PINS
 *   Right motor : ENA = 2 (PWM), IN1 = 12, IN2 = 11
 *   Left motor  : ENB = 3 (PWM), IN3 = 6,  IN4 = 7
 *   IR sensors  : Left = 30, Mid = 32, Right = 34
 *                 (moved off the IIT Delhi pins 49/51/53, which clash with SPI)
 *   NRF24L01    : CE = 48, CSN = 49, SCK = 52, MOSI = 51, MISO = 50
 *   MPU6500     : SDA = 20, SCL = 21
 *
 * Changes made when this was archived into the repo:
 *   - Dead-zone compensation added to setMotorSpeed() (as advised 8 Aug 2026).
 *   - LEFT_MOTOR_REVERSED flag added, same meaning as in firmware/balance.
 *   - Bug fix: imu.calibrateAccelGyro(200) -> calibrateAccelGyro(&calib) followed
 *     by init(calib), as FastIMU expects (see firmware/balance).
 *   - Bug fix: motorSpeedCtrl() sent left_speed to the RIGHT motor and
 *     right_speed to the LEFT motor (setMotorSpeed's bool means "isRight").
 *
 * PID units: this version converts the error to RADIANS and negates the
 * output, matching the IIT Delhi code. Gains here are NOT interchangeable
 * with firmware/balance, which works in degrees.
 */

#include <Wire.h>
#include <FastIMU.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// ==================== PIN ASSIGNMENTS ====================
#define ENA 2    // Right motor PWM
#define INA 12
#define INB 11
#define ENB 3    // Left motor PWM
#define INC 6
#define IND 7

#define PIN_LEFT   30
#define PIN_MID    32
#define PIN_RIGHT  34

#define NRF_CE   48
#define NRF_CSN  49

#define LEFT_MOTOR_REVERSED 0   // see firmware/balance for meaning
const int DEADZONE = 65;        // from firmware/tests/motor_deadzone_test

// ==================== IMU ====================
MPU6500   imu;
calData   calib = { 0 };
AccelData accelData;
GyroData  gyroData;

// ==================== RADIO ====================
RF24 radio(NRF_CE, NRF_CSN);
const byte address[6] = "00001";   // must match transmitter exactly

struct JoystickData {
  int  x;
  int  y;
  bool sw;
};

JoystickData joystick = {0, 0, false};

// ==================== MODE CONTROL ====================
bool manualMode = false;
unsigned long lastNRFReceive = 0;
#define NRF_TIMEOUT 500             // ms - fall back to IR mode if signal lost

bool lastSwState = false;
unsigned long lastSwToggleTime = 0;
#define SW_DEBOUNCE 300

// ==================== KALMAN FILTER ====================
class KalmanFilter {
public:
  float Q_angle   = 0.001;
  float Q_bias    = 0.003;
  float R_measure = 0.03;
  float angle = 0, bias = 0, rate = 0;
  float P[2][2] = {{0,0},{0,0}};

  float getAngle(float newAngle, float newRate, float dt) {
    rate    = newRate - bias;
    angle  += dt * rate;
    P[0][0] += dt * (dt*P[1][1] - P[1][0] - P[0][1] + Q_angle);
    P[0][1] -= dt * P[1][1];
    P[1][0] -= dt * P[1][1];
    P[1][1] += Q_bias * dt;
    float y = newAngle - angle;
    float S = P[0][0] + R_measure;
    float K0 = P[0][0] / S;
    float K1 = P[1][0] / S;
    angle += K0 * y;
    bias  += K1 * y;
    float P00 = P[0][0], P01 = P[0][1];
    P[0][0] -= K0 * P00;
    P[0][1] -= K0 * P01;
    P[1][0] -= K1 * P00;
    P[1][1] -= K1 * P01;
    return angle;
  }
};

KalmanFilter kalmanPitch;

// ==================== PID ====================
float Kp = 450.0, Ki = 750.0, Kd = 0.1;
float targetAngle = 0.0;
float error = 0, lastError = 0;
float integral = 0, derivative = 0;
float output = 0;
int   motorSpeed = 0;

// ==================== TIMING ====================
unsigned long currentTime = 0, lastTime = 0;
float dt = 0;

// ==================== TILT THRESHOLDS (deg) ====================
float low_threshold     = 1.2;
float mid_threshold     = 8.5;
float high_threshold    = 17.5;
float highest_threshold = 35.0;
int   tilt_marker = 0;

// ==================== MOTOR SPEED VARIABLES ====================
int left_speed = 0, right_speed = 0;
int straight_speed   = 60;
int low_turn_speed   = 50;
int sharp_turn_speed = 75;
int left_or_right    = 1;

// ==================== MANUAL OVERRIDE LIMITS ====================
#define MAX_MANUAL_FORWARD 80
#define MAX_MANUAL_TURN    60

// ==================== MOTOR FUNCTIONS ====================
void setupMotors() {
  pinMode(ENA, OUTPUT); pinMode(INA, OUTPUT); pinMode(INB, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(INC, OUTPUT); pinMode(IND, OUTPUT);
}

int applyDeadzone(int pwm) {
  if (abs(pwm) < 5) return 0;
  int sign = (pwm > 0) ? 1 : -1;
  return sign * map(abs(pwm), 5, 250, DEADZONE, 250);
}

void driveChannel(int speed, int inA, int inB, int en, bool reversed) {
  if (reversed) speed = -speed;
  if      (speed > 0) { digitalWrite(inA, LOW);  digitalWrite(inB, HIGH); }
  else if (speed < 0) { digitalWrite(inA, HIGH); digitalWrite(inB, LOW);  }
  else                { digitalWrite(inA, LOW);  digitalWrite(inB, LOW);  }
  analogWrite(en, abs(speed));
}

void setMotorSpeed(int speed, bool isRightMotor) {
  speed = constrain(speed, -250, 250);
  speed = applyDeadzone(speed);
  if (isRightMotor) driveChannel(speed, INA, INB, ENA, false);
  else              driveChannel(speed, INC, IND, ENB, LEFT_MOTOR_REVERSED);
}

// ==================== IR FUNCTIONS ====================
void setupIR() {
  pinMode(PIN_LEFT,  INPUT);
  pinMode(PIN_MID,   INPUT);
  pinMode(PIN_RIGHT, INPUT);
}

// Rule table from the IIT Delhi report
int lineChecker(bool L, bool M, bool R) {
  if (!L && !M &&  R) return 4; // sharp right
  if (!L &&  M && !R) return 1; // straight
  if (!L &&  M &&  R) return 5; // right
  if ( L && !M && !R) return 2; // sharp left
  if ( L &&  M && !R) return 3; // left
  return 0;                     // stability / stop
}

// ==================== MOTOR SPEED CONTROL ====================
void motorSpeedCtrl(int lineCase, int baseSpeed, int manualFwd, int manualTurn) {
  if (manualMode) {
    left_speed  = baseSpeed + manualFwd - manualTurn;
    right_speed = baseSpeed + manualFwd + manualTurn;
  } else {
    switch (lineCase) {
      case 1: // straight
        left_speed  = baseSpeed + (left_or_right * straight_speed);
        right_speed = baseSpeed + (left_or_right * straight_speed);
        break;
      case 2: // sharp left
        left_speed  = baseSpeed - (left_or_right * sharp_turn_speed);
        right_speed = baseSpeed + (left_or_right * sharp_turn_speed);
        break;
      case 3: // left
        left_speed  = baseSpeed - (left_or_right * low_turn_speed);
        right_speed = baseSpeed + (left_or_right * low_turn_speed);
        break;
      case 4: // sharp right
        left_speed  = baseSpeed + (left_or_right * sharp_turn_speed);
        right_speed = baseSpeed - (left_or_right * sharp_turn_speed);
        break;
      case 5: // right
        left_speed  = baseSpeed + (left_or_right * low_turn_speed);
        right_speed = baseSpeed - (left_or_right * low_turn_speed);
        break;
      default: // stability
        left_speed  = baseSpeed;
        right_speed = baseSpeed;
        break;
    }
  }
  setMotorSpeed(right_speed, true);
  setMotorSpeed(left_speed,  false);
}

// ==================== PID CONTROL ====================
void pid(float pitch, float dt) {
  float absPitch = abs(pitch);

  if (absPitch > mid_threshold && absPitch < high_threshold) {
    tilt_marker = 1;
    motorSpeed  = (pitch / absPitch) * 230;
    Serial.println("TILT: recovering (level 1)");
  }
  else if (absPitch >= high_threshold && absPitch < highest_threshold) {
    tilt_marker = 1;
    motorSpeed  = (pitch / absPitch) * 250;
    Serial.println("TILT: recovering (level 2)");
  }
  else if (absPitch >= highest_threshold) {
    tilt_marker = 1;
    motorSpeed  = 0;
    Serial.println("EMERGENCY STOP - tilt too large");
  }
  else if (absPitch > low_threshold && absPitch <= mid_threshold) {
    tilt_marker = 0;
    error      = (targetAngle - pitch) * PI / 180.0;
    integral  += Ki * error;
    integral   = constrain(integral, -5000, 5000); // anti-windup
    derivative = (error - lastError) / dt;
    output     = Kp * error + integral * dt + Kd * derivative;
    output    *= -1;
    lastError  = error;
    motorSpeed = constrain(output, -250, 250);
  }
  else {
    tilt_marker = 0;
    motorSpeed  = 0;
    integral    = 0;
    lastError   = 0;
  }
}

// ==================== NRF SETUP ====================
void setupNRF() {
  if (!radio.begin()) {
    Serial.println("NRF24L01 not detected on receiver - check wiring!");
    while (1);
  }
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_LOW);    // raise to RF24_PA_HIGH on BOTH ends once the link is reliable
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(76);
  radio.startListening();
  Serial.println("NRF24L01 receiver ready");
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);

  setupIR();
  setupMotors();
  setupNRF();

  int err = imu.init(calib, 0x68);
  if (err != 0) {
    Serial.print("IMU init failed, code: ");
    Serial.println(err);
    while (1);
  }
  Serial.println("IMU initialized");

  Serial.println("Calibrating - keep robot still and upright...");
  imu.calibrateAccelGyro(&calib);   // fills calib with accel/gyro biases
  imu.init(calib, 0x68);            // re-init so the biases are applied
  Serial.println("Calibration complete. Starting balance loop.");

  lastTime = millis();
  delay(300);
}

// ==================== MAIN LOOP ====================
void loop() {
  // ---- NRF receive ----
  if (radio.available()) {
    radio.read(&joystick, sizeof(joystick));
    lastNRFReceive = millis();

    // Non-blocking mode toggle on the rising edge of the button
    if (joystick.sw != lastSwState) {
      if (joystick.sw && (millis() - lastSwToggleTime > SW_DEBOUNCE)) {
        manualMode = !manualMode;
        lastSwToggleTime = millis();
        Serial.print("Mode switched: ");
        Serial.println(manualMode ? "MANUAL" : "IR LINE FOLLOW");
      }
      lastSwState = joystick.sw;
    }
  }

  // Safety fallback - no signal for 500 ms = IR mode
  if (millis() - lastNRFReceive > NRF_TIMEOUT) {
    manualMode = false;
    joystick.x = 0;
    joystick.y = 0;
  }

  // ---- IMU read ----
  imu.update();
  imu.getAccel(&accelData);
  imu.getGyro(&gyroData);

  currentTime = millis();
  dt = (currentTime - lastTime) / 1000.0;
  lastTime = currentTime;
  if (dt <= 0) dt = 0.001;

  float ax = accelData.accelX;
  float ay = accelData.accelY;
  float az = accelData.accelZ;
  float gx = gyroData.gyroX;

  float pitchAcc = atan2(ay, sqrt(ax*ax + az*az)) * 180.0 / PI;
  float pitch = kalmanPitch.getAngle(pitchAcc, gx, dt);

  // ---- PID ----
  pid(pitch, dt);

  // ---- Map joystick ----
  int manualFwd  = map(joystick.y, -255, 255, -MAX_MANUAL_FORWARD, MAX_MANUAL_FORWARD);
  int manualTurn = map(joystick.x, -255, 255, -MAX_MANUAL_TURN, MAX_MANUAL_TURN);

  // ---- Direction logic ----
  int lineCase = 0;
  if (!manualMode && tilt_marker == 0) {
    lineCase = lineChecker(digitalRead(PIN_LEFT), digitalRead(PIN_MID), digitalRead(PIN_RIGHT));
  }

  // ---- Apply motor commands ----
  if (tilt_marker == 1) {
    setMotorSpeed(motorSpeed, true);
    setMotorSpeed(motorSpeed, false);
  } else {
    motorSpeedCtrl(lineCase, motorSpeed, manualFwd, manualTurn);
  }

  // ---- Debug ----
  Serial.print("Pitch: ");   Serial.print(pitch, 2);
  Serial.print(" | Mode: "); Serial.print(manualMode ? "MANUAL" : "IR");
  Serial.print(" | L: ");    Serial.print(left_speed);
  Serial.print(" | R: ");    Serial.println(right_speed);
}
