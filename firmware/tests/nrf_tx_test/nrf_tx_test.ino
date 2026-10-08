// NRF24L01 link test - TRANSMITTER (Nano) (24 Jun 2026).
// Nano: CE D9, CSN D10, SCK D13, MOSI D11, MISO D12, VCC 3V3 + 100 uF cap.
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(9, 10);
const byte address[6] = "00001";

struct JoystickData {
  int x;
  int y;
  bool sw;
};

JoystickData data;

void setup() {
  Serial.begin(115200);
  if (!radio.begin()) {
    Serial.println("NRF NOT DETECTED - check wiring");
    while (1);
  }
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(76);
  radio.stopListening();
  Serial.println("Transmitter ready");
}

void loop() {
  data.x  = analogRead(A0);
  data.y  = analogRead(A1);
  data.sw = false;

  bool ok = radio.write(&data, sizeof(data));
  Serial.print("X: ");  Serial.print(data.x);
  Serial.print(" Y: "); Serial.print(data.y);
  Serial.print(" Sent: "); Serial.println(ok ? "OK" : "FAIL");
  delay(100);
}
