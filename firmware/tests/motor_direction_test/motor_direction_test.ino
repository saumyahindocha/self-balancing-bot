// Motor direction check (18 Aug 2026), using the same motor code as
// firmware/balance. Both wheels should roll FORWARD together for 1 s, then
// BACKWARD together for 1 s, then stop.
// If the left wheel goes the other way: swap its wires on OUT3/OUT4, or set
// LEFT_MOTOR_REVERSED to 1 here AND in firmware/balance.
#define LEFT_MOTOR_REVERSED 0

#define ENA 2
#define INA 12
#define INB 11
#define ENB 3
#define INC 6
#define IND 7

void driveChannel(int speed, int inA, int inB, int en, bool reversed) {
  if (reversed) speed = -speed;
  if      (speed > 0) { digitalWrite(inA, LOW);  digitalWrite(inB, HIGH); }
  else if (speed < 0) { digitalWrite(inA, HIGH); digitalWrite(inB, LOW);  }
  else                { digitalWrite(inA, LOW);  digitalWrite(inB, LOW);  }
  analogWrite(en, abs(speed));
}

void setBoth(int speed) {
  driveChannel(speed, INA, INB, ENA, false);                 // right
  driveChannel(speed, INC, IND, ENB, LEFT_MOTOR_REVERSED);   // left
}

void setup() {
  Serial.begin(115200);
  pinMode(ENA, OUTPUT); pinMode(INA, OUTPUT); pinMode(INB, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(INC, OUTPUT); pinMode(IND, OUTPUT);
}

void loop() {
  Serial.println("FORWARD");  setBoth(150);  delay(1000);
  Serial.println("BACKWARD"); setBoth(-150); delay(1000);
  Serial.println("STOP");     setBoth(0);    delay(500);
}
