/*
 * NIDS Two-Wheeled Self-Balancing Robot
 * firmware/balance/balance.ino
 *
 * Current build: balance only. The NRF24L01 remote and IR line following were
 * shelved on 18 Aug 2026 to focus on getting the robot to stand up. Their code
 * is kept in firmware/archive/.
 *
 * Board     : Arduino Mega 2560
 * Libraries : FastIMU (LiquidCGS) from Library Manager, Wire (built in)
 * Serial    : 115200 baud
 *
 * PINS
 *   Right motor (L298N A) : ENA = 2 (PWM), IN1 = 12, IN2 = 11
 *   Left motor  (L298N B) : ENB = 3 (PWM), IN3 = 6,  IN4 = 7
 *   MPU6500               : SDA = 20, SCL = 21 (hardware I2C), address 0x68
 *
 * BEFORE YOU RUN THIS
 *   1. Remove the ENA/ENB jumpers on the L298N and wire them to pins 2 and 3.
 *   2. Tie L298N GND to Mega GND (common ground). Power the Mega from USB.
 *   3. Run firmware/tests/motor_deadzone_test and put the result in DEADZONE.
 *   4. Hold the robot still and upright while it calibrates at power-up (~4 s).
 *   5. Keep the 3S LiPo above ~10.5 V while tuning (see docs/tuning.md).
 *
 * TUNING ORDER (from docs/tuning.md)
 *   Kp = 30, Ki = 0, Kd = 0  ->  raise Kp until it fights back but oscillates
 *   add Kd (0.5 to 2.0)      ->  until the oscillation is damped
 *   add Ki (10 to 50) last   ->  until slow drift is gone
 *
 * Fixed when moved into the repo: the chat version called
 * imu.calibrateAccelGyro(200). FastIMU's function takes a calData*, so 200 was
 * treated as a memory address and the biases were never applied. It now
 * calibrates into `calib` and re-runs init() so the biases take effect.
 *
 * Units: pitch and error are in DEGREES here. The IIT Delhi gains
 * (Kp 441 / Ki 750 / Kd 0.01) and the MATLAB gains were for an error in
 * RADIANS, so they are about 57x too aggressive if pasted into this sketch.
 */

#include <Wire.h>
#include <FastIMU.h>

// ---- TUNE THESE -------------------------------------------------------------
float Kp          = 30.0;
float Ki          = 0.0;
float Kd          = 0.0;
float targetAngle = 0.0;   // pitch (deg) at which the robot balances
const int DEADZONE = 65;   // lowest PWM at which your motors start to turn
// -----------------------------------------------------------------------------

// The two motors are mirror-mounted, so the same command spins them opposite
// ways. Fix it ONE way only:
//   - swap the two left-motor wires on L298N OUT3/OUT4 and leave this at 0, or
//   - leave the wires and set this to 1.
#define LEFT_MOTOR_REVERSED 0

// Motor pins
#define ENA 2
#define INA 12
#define INB 11
#define ENB 3
#define INC 6
#define IND 7

// Safety limits (degrees)
const float RECOVERY_ANGLE = 17.5;  // beyond this: full-power recovery
const float FALLEN_ANGLE   = 35.0;  // beyond this: motors off

// IMU
MPU6500   imu;
calData   calib = {0};
AccelData accelData;
GyroData  gyroData;

// ---- Kalman filter (angle + gyro bias), same as the IIT Delhi reference -----
class KalmanFilter {
public:
  float Q_angle   = 0.001;
  float Q_bias    = 0.003;
  float R_measure = 0.03;
  float angle = 0, bias = 0;
  float P[2][2] = {{0, 0}, {0, 0}};

  float getAngle(float newAngle, float newRate, float dt) {
    float rate = newRate - bias;
    angle += dt * rate;

    P[0][0] += dt * (dt * P[1][1] - P[1][0] - P[0][1] + Q_angle);
    P[0][1] -= dt * P[1][1];
    P[1][0] -= dt * P[1][1];
    P[1][1] += Q_bias * dt;

    float y  = newAngle - angle;
    float S  = P[0][0] + R_measure;
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

KalmanFilter kalman;

// PID state
float error = 0, lastError = 0, integral = 0;
int   motorSpeed = 0;
unsigned long lastTime = 0;

// ---- Dead-zone compensation -------------------------------------------------
// Below DEADZONE the motors do not turn at all, so small PID corrections would
// be ignored. Map commands 5..250 onto DEADZONE..250; |cmd| < 5 means stop.
int applyDeadzone(int pwm) {
  if (abs(pwm) < 5) return 0;
  int sign = (pwm > 0) ? 1 : -1;
  return sign * map(abs(pwm), 5, 250, DEADZONE, 250);
}

// ---- Motor driver -----------------------------------------------------------
void driveChannel(int speed, int inA, int inB, int en, bool reversed) {
  if (reversed) speed = -speed;
  if      (speed > 0) { digitalWrite(inA, LOW);  digitalWrite(inB, HIGH); }
  else if (speed < 0) { digitalWrite(inA, HIGH); digitalWrite(inB, LOW);  }
  else                { digitalWrite(inA, LOW);  digitalWrite(inB, LOW);  }
  analogWrite(en, abs(speed));
}

void setMotorSpeed(int speed, bool isRight) {
  speed = constrain(speed, -250, 250);
  speed = applyDeadzone(speed);
  if (isRight) driveChannel(speed, INA, INB, ENA, false);
  else         driveChannel(speed, INC, IND, ENB, LEFT_MOTOR_REVERSED);
}

void stopMotors() {
  setMotorSpeed(0, true);
  setMotorSpeed(0, false);
}

// ---- Setup ------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);

  pinMode(ENA, OUTPUT); pinMode(INA, OUTPUT); pinMode(INB, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(INC, OUTPUT); pinMode(IND, OUTPUT);
  stopMotors();

  int err = imu.init(calib, 0x68);
  if (err != 0) {
    Serial.print("IMU init failed, error: ");
    Serial.println(err);
    while (1);
  }

  Serial.println("Calibrating - hold robot STILL and UPRIGHT...");
  imu.calibrateAccelGyro(&calib);   // fills calib with accel/gyro biases
  imu.init(calib, 0x68);            // re-init so the biases are applied
  Serial.println("Done. Balancing...");

  lastTime = millis();
}

// ---- Main loop --------------------------------------------------------------
void loop() {
  // 1. Read IMU
  imu.update();
  imu.getAccel(&accelData);
  imu.getGyro(&gyroData);

  unsigned long now = millis();
  float dt = (now - lastTime) / 1000.0;
  if (dt <= 0) dt = 0.001;
  lastTime = now;

  float ax = accelData.accelX;
  float ay = accelData.accelY;
  float az = accelData.accelZ;
  float gx = gyroData.gyroX;          // deg/s

  // 2. Estimate pitch (deg)
  float pitchAcc = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI;
  float pitch    = kalman.getAngle(pitchAcc, gx, dt);

  // 3. Safety: fallen over -> stop and reset the controller
  if (abs(pitch) > FALLEN_ANGLE) {
    stopMotors();
    integral  = 0;
    lastError = 0;
    Serial.println("FALLEN - stopped");
    return;
  }

  // 4. Large tilt -> full-power recovery towards upright
  if (abs(pitch) > RECOVERY_ANGLE) {
    int sign = (pitch > 0) ? 1 : -1;
    setMotorSpeed(-sign * 250, true);
    setMotorSpeed(-sign * 250, false);
    return;
  }

  // 5. PID
  error     = targetAngle - pitch;
  integral += Ki * error * dt;
  integral  = constrain(integral, -5000.0, 5000.0);   // anti-windup
  float deriv  = (error - lastError) / dt;
  float output = Kp * error + integral + Kd * deriv;
  lastError = error;

  motorSpeed = constrain((int)output, -250, 250);

  // 6. Drive both wheels with the same command
  setMotorSpeed(motorSpeed, true);
  setMotorSpeed(motorSpeed, false);

  // 7. Debug
  Serial.print("Pitch: ");    Serial.print(pitch, 2);
  Serial.print(" | Error: "); Serial.print(error, 2);
  Serial.print(" | PWM: ");   Serial.println(motorSpeed);
}
