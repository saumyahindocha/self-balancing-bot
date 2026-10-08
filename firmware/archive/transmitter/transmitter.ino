/*
 * NIDS remote transmitter (ARCHIVED)
 * firmware/archive/transmitter/transmitter.ino
 *
 * STATUS: shelved with the NRF24L01 link on 18 Aug 2026. Compiled cleanly on
 * 21 Jun 2026; the remote itself was built on perfboard inside a 3D-printed
 * pistol-grip enclosure.
 *
 * Board   : Arduino Nano (ATmega328P)
 * Library : RF24 (TMRh20)
 *
 * PINS (from the EasyEDA schematic of the remote, CE/CSN confirmed 2 Jun 2026)
 *   NRF24L01 (PA+LNA) : CE = D9, CSN = D10, SCK = D13, MOSI = D11, MISO = D12,
 *                       VCC = Nano 3V3 (100 nF + 100 uF across VCC/GND)
 *   KY-023 joystick   : VRX = A0, VRY = A1, SW = D2, VCC = 5V
 *   Mode switch       : between D2 and GND (in parallel with the joystick
 *                       button; either one toggles IR <-> MANUAL on the robot)
 *   Power             : 9 V battery -> Nano VIN
 *
 * Packet: { int x; int y; bool sw; } at ~50 Hz, x/y in -255..255 with a
 * dead-band around centre. Address "00001", channel 76, 250 kbps.
 */

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define NRF_CE   9
#define NRF_CSN  10

#define JOY_VRX  A0
#define JOY_VRY  A1
#define JOY_SW   2

RF24 radio(NRF_CE, NRF_CSN);
const byte address[6] = "00001";   // must match the receiver exactly

struct JoystickData {
  int  x;     // -255 to 255, left/right
  int  y;     // -255 to 255, forward/back
  bool sw;    // button / mode switch
};

JoystickData data;

#define JOY_CENTER   512
#define JOY_DEADBAND 40

// Non-blocking button debounce
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
#define DEBOUNCE_DELAY 50

int mapWithDeadband(int rawVal) {
  if (abs(rawVal - JOY_CENTER) < JOY_DEADBAND) return 0;
  if (rawVal < JOY_CENTER) return map(rawVal, 0, JOY_CENTER, -255, 0);
  return map(rawVal, JOY_CENTER, 1023, 0, 255);
}

void setup() {
  Serial.begin(115200);
  pinMode(JOY_SW, INPUT_PULLUP);

  if (!radio.begin()) {
    Serial.println("NRF24L01 not detected - check wiring!");
    while (1);
  }

  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW);     // raise to RF24_PA_HIGH on BOTH ends once reliable
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(76);
  radio.stopListening();

  Serial.println("Transmitter ready");
}

void loop() {
  data.x = mapWithDeadband(analogRead(JOY_VRX));
  data.y = mapWithDeadband(analogRead(JOY_VRY));

  bool reading = digitalRead(JOY_SW);
  if (reading != lastButtonState) lastDebounceTime = millis();
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    data.sw = (reading == LOW);       // pressed = LOW (INPUT_PULLUP)
  }
  lastButtonState = reading;

  bool success = radio.write(&data, sizeof(data));

  Serial.print("X: ");     Serial.print(data.x);
  Serial.print(" Y: ");    Serial.print(data.y);
  Serial.print(" SW: ");   Serial.print(data.sw);
  Serial.print(" Sent: "); Serial.println(success ? "OK" : "FAIL");

  delay(20); // ~50 Hz
}
