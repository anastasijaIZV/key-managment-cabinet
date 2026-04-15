#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_PN532.h>

#define SDA_PIN 21
#define SCL_PIN 22

Adafruit_PN532 nfc(-1, -1);

const char* ssid = "Intergalactic wi-fi";
const char* password = "theansweris42";
String scriptURL = "https://script.google.com/macros/s/AKfycbyX63V4DyRIOaheHBSpp5F0Qq8yIyo5oV7M2HlPrb7_t-Lq6zlBEhbJV_SD3D7FAvPW/exec";

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  // WiFi connect
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");

  // NFC init
  nfc.begin();
  if (!nfc.getFirmwareVersion()) {
    Serial.println("PN532 not found");
    while (1);
  }

  nfc.SAMConfig();
  Serial.println("Ready to scan...");
}

void loop() {
  uint8_t uid[7];
  uint8_t uidLength;

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {

    String uidStr = "";

    for (uint8_t i = 0; i < uidLength; i++) {
      uidStr += String(uid[i], HEX);
    }

    Serial.println("UID: " + uidStr);

    sendToGoogle(uidStr);

    delay(2000); // prevent spam
  }
}

void sendToGoogle(String uid) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url = scriptURL + "?uid=" + uid;

    http.begin(url);
    int httpCode = http.GET();

    Serial.print("HTTP Response: ");
    Serial.println(httpCode);

    http.end();
  }
}