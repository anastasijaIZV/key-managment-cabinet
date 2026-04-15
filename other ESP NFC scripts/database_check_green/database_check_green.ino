#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_PN532.h>

#define SDA_PIN 21
#define SCL_PIN 22

#define GREEN_LED 23
#define RED_LED 12

const char* ssid = "Intergalactic wi-fi";
const char* password = "theansweris42";

// Paste your deployed Apps Script web app URL here
String scriptURL = "https://script.google.com/macros/s/AKfycbw6tF4ynB5MSCIu27RRfJoWgRToa7kV3A6Q1mWtgyPa2j50o4haVSbWZCjxVZzH2c1m/exec";

Adafruit_PN532 nfc(-1, -1);

void showGreen() {
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, HIGH);
  delay(700);
  digitalWrite(GREEN_LED, LOW);
}

void showRed() {
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);
  delay(700);
  digitalWrite(RED_LED, LOW);
}

String uidToString(uint8_t *uid, uint8_t uidLength) {
  String result = "";
  for (uint8_t i = 0; i < uidLength; i++) {
    if (uid[i] < 0x10) result += "0";
    result += String(uid[i], HEX);
  }
  result.toUpperCase();
  return result;
}

String checkUIDInSheet(String uid) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    return "ERROR";
  }

  HTTPClient http;
  String url = scriptURL + "?uid=" + uid;

  Serial.print("Requesting: ");
  Serial.println(url);

  http.begin(url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    payload.trim();

    Serial.print("HTTP code: ");
    Serial.println(httpCode);

    Serial.print("Server response: ");
    Serial.println(payload);

    http.end();
    return payload;
  } else {
    Serial.print("HTTP error: ");
    Serial.println(httpCode);
    http.end();
    return "ERROR";
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);

  Wire.begin(SDA_PIN, SCL_PIN);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("PN532 not found");
    while (1) {
      showRed();
      delay(500);
    }
  }

  nfc.SAMConfig();
  Serial.println("PN532 ready");
  Serial.println("Waiting for NFC card...");
}

void loop() {
  uint8_t uid[7];
  uint8_t uidLength;

  bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 200);

  if (success) {
    String uidString = uidToString(uid, uidLength);

    Serial.print("Card UID: ");
    Serial.println(uidString);

    String result = checkUIDInSheet(uidString);

      if (result == "FOUND") {
    Serial.println("Access granted");
    showGreen();
  } else if (result == "NOT_FOUND" || result == "INACTIVE") {
    Serial.println("Access denied");
    showRed();
  } else {
    Serial.println("Server/WiFi error");
    showRed();
    delay(200);
    showRed();
  }

    delay(1500);
  }
}