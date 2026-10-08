// L298N check (24 Jun 2026): forward / stop / reverse / stop at half speed.
// ENA/ENB jumpers must be removed and wired to pins 2 / 3.
#define ENA 2
#define INA 12
#define INB 11
#define ENB 3
#define INC 6
#define IND 7

void setup() {
  pinMode(ENA, OUTPUT); pinMode(INA, OUTPUT); pinMode(INB, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(INC, OUTPUT); pinMode(IND, OUTPUT);
  Serial.begin(115200);
  Serial.println("L298N test starting");
}

void loop() {
  Serial.println("FORWARD - half speed");
  digitalWrite(INA, LOW); digitalWrite(INB, HIGH); analogWrite(ENA, 128);
  digitalWrite(INC, LOW); digitalWrite(IND, HIGH); analogWrite(ENB, 128);
  delay(2000);

  Serial.println("STOP");
  analogWrite(ENA, 0); analogWrite(ENB, 0);
  delay(1000);

  Serial.println("REVERSE - half speed");
  digitalWrite(INA, HIGH); digitalWrite(INB, LOW); analogWrite(ENA, 128);
  digitalWrite(INC, HIGH); digitalWrite(IND, LOW); analogWrite(ENB, 128);
  delay(2000);

  Serial.println("STOP");
  analogWrite(ENA, 0); analogWrite(ENB, 0);
  delay(1000);
}
