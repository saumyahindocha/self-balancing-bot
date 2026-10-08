// MPU6500 check (24 Jun 2026). Mega: SDA 20, SCL 21, VCC 3.3V.
// error -1 -> check SDA/SCL wiring; error -2 -> try address 0x69.
#include <Wire.h>
#include <FastIMU.h>

MPU6500 imu;
calData calib = {0};
AccelData accelData;
GyroData gyroData;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);

  int err = imu.init(calib, 0x68);
  if (err != 0) {
    Serial.print("IMU failed, code: ");
    Serial.println(err);
    while (1);
  }
  Serial.println("IMU OK - calibrating, keep still...");
  imu.calibrateAccelGyro(&calib);   // fills calib with accel/gyro biases
  imu.init(calib, 0x68);            // re-init so the biases are applied
  Serial.println("Done - tilt me around!");
}

void loop() {
  imu.update();
  imu.getAccel(&accelData);
  imu.getGyro(&gyroData);

  float ax = accelData.accelX;
  float ay = accelData.accelY;
  float az = accelData.accelZ;
  float pitch = atan2(ay, sqrt(ax*ax + az*az)) * 180.0 / PI;

  Serial.print("Pitch: "); Serial.print(pitch, 2);
  Serial.print(" | AX: "); Serial.print(ax, 2);
  Serial.print(" AY: ");   Serial.print(ay, 2);
  Serial.print(" AZ: ");   Serial.println(az, 2);
  delay(50);
}
