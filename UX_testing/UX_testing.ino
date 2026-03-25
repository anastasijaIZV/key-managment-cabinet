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
String scriptURL = "https://script.google.com/macros/s/AKfycbxJ4BgzgVrgRWvFc7PEc3DR30w1ZKmYadbgoWtj0ghi8KjdBrXuXU6r1yyRBP6yyog3/exec";

Adafruit_PN532 nfc(-1, -1);

// Pending transaction state
String pendingAction = "";   // TAKE or RETURN
String pendingUserUID = "";
String pendingUserName = "";
String pendingKeyUID = "";
String pendingKeyName = "";
unsigned long pendingSince = 0;
const unsigned long PENDING_TIMEOUT_MS = 10000;

void allOff() {
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
}

void blinkGreen(int count, int onMs = 180, int offMs = 120) {
  for (int i = 0; i < count; i++) {
    allOff();
    digitalWrite(GREEN_LED, HIGH);
    delay(onMs);
    digitalWrite(GREEN_LED, LOW);
    delay(offMs);
  }
}

void blinkRed(int count, int onMs = 180, int offMs = 120) {
  for (int i = 0; i < count; i++) {
    allOff();
    digitalWrite(RED_LED, HIGH);
    delay(onMs);
    digitalWrite(RED_LED, LOW);
    delay(offMs);
  }
}

void showGreenOne() {
  blinkGreen(1);
}

void showGreenThree() {
  blinkGreen(3);
}

void showRedOne() {
  blinkRed(1);
}

void showRedThree() {
  blinkRed(3);
}

void clearPending() {
  pendingAction = "";
  pendingUserUID = "";
  pendingUserName = "";
  pendingKeyUID = "";
  pendingKeyName = "";
  pendingSince = 0;
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

String httpGet(String url) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    return "ERROR";
  }

  HTTPClient http;
  http.begin(url);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(5000);

  int httpCode = http.GET();
  String payload = "";

  if (httpCode > 0) {
    payload = http.getString();
    payload.trim();
    Serial.print("HTTP ");
    Serial.println(httpCode);
    Serial.print("Response: ");
    Serial.println(payload);
  } else {
    Serial.print("HTTP error: ");
    Serial.println(httpCode);
    payload = "ERROR";
  }

  http.end();
  return payload;
}

String classifyUid(String uid) {
  String url = scriptURL + "?mode=classify&uid=" + uid;
  Serial.print("Classify URL: ");
  Serial.println(url);
  return httpGet(url);
}

String processAction(String action, String userUid, String keyUid) {
  String url = scriptURL + "?mode=process&action=" + action + "&useruid=" + userUid + "&keyuid=" + keyUid;
  Serial.print("Process URL: ");
  Serial.println(url);
  return httpGet(url);
}

String getPart(String data, int index) {
  int start = 0;
  int currentIndex = 0;

  for (int i = 0; i <= data.length(); i++) {
    if (i == data.length() || data.charAt(i) == '|') {
      if (currentIndex == index) {
        return data.substring(start, i);
      }
      currentIndex++;
      start = i + 1;
    }
  }
  return "";
}

void handleUserFound(String uid, String name) {
  if (pendingAction == "") {
    // First scan is user -> TAKE
    pendingAction = "TAKE";
    pendingUserUID = uid;
    pendingUserName = name;
    pendingSince = millis();

    Serial.println("User scanned first -> pending TAKE");
    Serial.print("User: ");
    Serial.println(name);
    showGreenOne();
    return;
  }

  if (pendingAction == "RETURN" && pendingKeyUID != "") {
    // Second scan completes RETURN
    Serial.println("Completing RETURN...");
    String result = processAction("RETURN", uid, pendingKeyUID);

    if (result.startsWith("OK|RETURN|")) {
      Serial.println("RETURN logged successfully");
      Serial.println(result);
      showGreenThree();
      clearPending();
    } else if (
      result == "KEY_ALREADY_IN" ||
      result == "USER_NOT_FOUND" ||
      result == "USER_INACTIVE" ||
      result == "KEY_NOT_FOUND"
    ) {
      Serial.println("RETURN denied");
      Serial.println(result);
      showRedOne();
      clearPending();
    } else {
      Serial.println("RETURN failed");
      Serial.println(result);
      showRedOne();
      clearPending();
    }
    return;
  }

  Serial.println("Unexpected user scan");
  showRedOne();
  clearPending();
}

void handleKeyFound(String uid, String name, String status, String holderUid) {
  if (pendingAction == "") {
    // First scan is key -> RETURN
    // Only allow this flow if key is currently OUT
    if (status != "OUT") {
      Serial.println("Key already in cabinet, cannot start RETURN");
      showRedOne();
      clearPending();
      return;
    }

    pendingAction = "RETURN";
    pendingKeyUID = uid;
    pendingKeyName = name;
    pendingSince = millis();

    Serial.println("Key scanned first -> pending RETURN");
    Serial.print("Key: ");
    Serial.println(name);
    showGreenOne();
    return;
  }

  if (pendingAction == "TAKE" && pendingUserUID != "") {
    // Second scan completes TAKE
    Serial.println("Completing TAKE...");
    String result = processAction("TAKE", pendingUserUID, uid);

    if (result.startsWith("OK|TAKE|")) {
      Serial.println("TAKE logged successfully");
      Serial.println(result);
      showGreenThree();
      clearPending();
    } else if (
      result == "KEY_NOT_AVAILABLE" ||
      result == "USER_NOT_FOUND" ||
      result == "USER_INACTIVE" ||
      result == "KEY_NOT_FOUND"
    ) {
      Serial.println("TAKE denied");
      Serial.println(result);
      showRedOne();
      clearPending();
    } else {
      Serial.println("TAKE failed");
      Serial.println(result);
      showRedOne();
      clearPending();
    }
    return;
  }

  Serial.println("Unexpected key scan");
  showRedOne();
  clearPending();
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  allOff();

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
      showRedOne();
      delay(1000);
    }
  }

  nfc.SAMConfig();
  clearPending();

  Serial.println("PN532 ready");
  Serial.println("Scan user first for TAKE, or key first for RETURN");
}

void loop() {
  if (pendingAction != "" && millis() - pendingSince > PENDING_TIMEOUT_MS) {
    Serial.println("Pending action timed out");
    showRedThree();
    clearPending();
  }

  uint8_t uid[7];
  uint8_t uidLength;

  bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 200);

  if (!success) {
    return;
  }

  String uidString = uidToString(uid, uidLength);
  Serial.print("Scanned UID: ");
  Serial.println(uidString);

  String result = classifyUid(uidString);

  if (
    result == "NOT_FOUND" ||
    result == "USER_INACTIVE" ||
    result == "DUPLICATE_UID" ||
    result == "ERROR"
  ) {
    Serial.println("Classify failed / access denied");
    Serial.println(result);
    showRedOne();
    delay(1200);
    return;
  }

  if (result.startsWith("USER_FOUND|")) {
    String foundUid = getPart(result, 1);
    String userName = getPart(result, 2);
    handleUserFound(foundUid, userName);
    delay(1200);
    return;
  }

  if (result.startsWith("KEY_FOUND|")) {
    String foundUid = getPart(result, 1);
    String keyName = getPart(result, 2);
    String status = getPart(result, 3);
    String holderUid = getPart(result, 4);
    handleKeyFound(foundUid, keyName, status, holderUid);
    delay(1200);
    return;
  }

  Serial.println("Unexpected server response");
  Serial.println(result);
  showRedOne();
  delay(1200);
}