// Motor dead-zone finder (8 Aug 2026).
// Steps the RIGHT motor (OUT1/OUT2) from PWM 0 to 150 in steps of 5, every
// 0.8 s. Note the first PWM at which the motor turns and copy it into
// DEADZONE in firmware/balance/balance.ino.
// Robot on its side or wheels off the table - nothing is balancing it.
#define ENA 2
#define INA 12
#define INB 11

void setup() {
  Serial.begin(115200);
  pinMode(ENA, OUTPUT);
  pinMode(INA, OUTPUT);
  pinMode(INB, OUTPUT);
  Serial.println("Dead-zone test: note the PWM where the motor FIRST moves.");
}

void loop() {
  for (int pwm = 0; pwm <= 150; pwm += 5) {
    digitalWrite(INA, LOW);
    digitalWrite(INB, HIGH);
    analogWrite(ENA, pwm);
    Serial.print("PWM: ");
    Serial.println(pwm);
    delay(800);
  }
  analogWrite(ENA, 0);
  Serial.println("--- cycle complete, restarting ---");
  delay(2000);
}
