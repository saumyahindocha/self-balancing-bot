// IR line sensor check (24 Jun 2026). Mega pins 30 / 32 / 34, sensors on 5V.
#define PIN_LEFT  30
#define PIN_MID   32
#define PIN_RIGHT 34

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LEFT,  INPUT);
  pinMode(PIN_MID,   INPUT);
  pinMode(PIN_RIGHT, INPUT);
  Serial.println("IR Test ready - place over black line");
}

void loop() {
  bool L = digitalRead(PIN_LEFT);
  bool M = digitalRead(PIN_MID);
  bool R = digitalRead(PIN_RIGHT);

  Serial.print("L:");  Serial.print(L);
  Serial.print(" M:"); Serial.print(M);
  Serial.print(" R:"); Serial.print(R);
  Serial.print(" | ");

  if      (!L &&  M && !R) Serial.println("STRAIGHT");
  else if ( L &&  M && !R) Serial.println("LEFT");
  else if (!L &&  M &&  R) Serial.println("RIGHT");
  else if ( L && !M && !R) Serial.println("SHARP LEFT");
  else if (!L && !M &&  R) Serial.println("SHARP RIGHT");
  else                     Serial.println("STABILITY");

  delay(100);
}
