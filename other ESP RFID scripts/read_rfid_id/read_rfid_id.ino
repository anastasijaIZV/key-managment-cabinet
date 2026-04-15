#include <HardwareSerial.h>

HardwareSerial RFID(2);

void setup() {
  Serial.begin(115200);
  RFID.begin(9600, SERIAL_8N1, 14, -1);
  Serial.println("Scan RFID card...");
}

void loop() {
  while (RFID.available()) {
    char c = RFID.read();
    Serial.write(c);
  }
}