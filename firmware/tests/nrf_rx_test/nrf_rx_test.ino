// NRF24L01 link test - RECEIVER (Mega) (24 Jun 2026).
// Mega: CE 48, CSN 49, SCK 52, MOSI 51, MISO 50, VCC 3.3V + 100 uF cap.
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(48, 49);
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
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(76);
  radio.startListening();
  Serial.println("Receiver ready");
}

void loop() {
  if (radio.available()) {
    radio.read(&data, sizeof(data));
    Serial.print("X: ");   Serial.print(data.x);
    Serial.print(" Y: ");  Serial.print(data.y);
    Serial.print(" SW: "); Serial.println(data.sw);
  }
}
