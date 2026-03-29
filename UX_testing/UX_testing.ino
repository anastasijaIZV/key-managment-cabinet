#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_PN532.h>

// I2C pins for PN532
#define SDA_PIN 21
#define SCL_PIN 22

// LED pins
#define GREEN_LED 23
#define RED_LED 12

// WiFi credentials (CHANGE THESE)
const char* ssid = "--------";
const char* password = "---------";

// Google Apps Script URL
String scriptURL = "---------";

Adafruit_PN532 nfc(-1, -1);

// Pending transaction state
String pendingAction = "";   // TAKE or RETURN
String pendingUserUID = "";
String pendingUserName = "";
String pendingKeyUID = "";
String pendingKeyName = "";

// Timestamp for timeout
unsigned long pendingSince = 0;
const unsigned long PENDING_TIMEOUT_MS = 10000;

void allOff() {
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
}

// Blink green LED
void blinkGreen(int count, int onMs = 180, int offMs = 120) {
  for (int i = 0; i < count; i++) {
    allOff();
    digitalWrite(GREEN_LED, HIGH);
    delay(onMs);
    digitalWrite(GREEN_LED, LOW);
    delay(offMs);
  }
}

// Blink red LED
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

// Reset current transaction
void clearPending() {
  pendingAction = "";
  pendingUserUID = "";
  pendingUserName = "";
  pendingKeyUID = "";
  pendingKeyName = "";
  pendingSince = 0;
}

// -----------------------------
// UID PROCESSING
// -----------------------------
// Converts raw UID bytes into readable HEX string
String uidToString(uint8_t *uid, uint8_t uidLength) {
  String result = "";
  for (uint8_t i = 0; i < uidLength; i++) {
    if (uid[i] < 0x10) result += "0";
    result += String(uid[i], HEX);
  }
  result.toUpperCase();
  return result;
}

// -----------------------------
// HTTP COMMUNICATION
// -----------------------------
// Sends GET request to Apps Script
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

// Ask backend: is this UID a user or a key?
String classifyUid(String uid) {
  String url = scriptURL + "?mode=classify&uid=" + uid;
  Serial.print("Classify URL: ");
  Serial.println(url);
  return httpGet(url);
}

// Tell backend to process TAKE or RETURN
String processAction(String action, String userUid, String keyUid) {
  String url = scriptURL + "?mode=process&action=" + action + "&useruid=" + userUid + "&keyuid=" + keyUid;
  Serial.print("Process URL: ");
  Serial.println(url);
  return httpGet(url);
}

// -----------------------------
// RESPONSE PARSING
// -----------------------------
// Splits server response using "|" separator
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

// -----------------------------
// LOGIC: USER SCANNED
// -----------------------------
void handleUserFound(String uid, String name) {
  if (pendingAction == "") {
    // FIRST SCAN → USER → start TAKE
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
    // SECOND SCAN → complete RETURN
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

// -----------------------------
// LOGIC: KEY SCANNED
// -----------------------------
void handleKeyFound(String uid, String name, String status, String holderUid) {
  if (pendingAction == "") {
    // FIRST SCAN → KEY → start RETURN
    if (status != "OUT") {
        // Key is already in cabinet
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
    // SECOND SCAN → complete TAKE
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
  // Wrong order
  Serial.println("Unexpected key scan");
  showRedOne();
  clearPending();
}

// -----------------------------
// SETUP
// -----------------------------
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  allOff();

  // Initialize I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
  
  // Initialize PN532
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();

  if (!versiondata) {
    Serial.println("PN532 not found");
    while (1) {
      showRedOne();
      delay(1000);
    }
  }

  nfc.SAMConfig(); // Enable NFC reader
  clearPending();

  Serial.println("PN532 ready");
  Serial.println("Scan user first for TAKE, or key first for RETURN");
}

void loop() {
  
    // Timeout if user doesn't complete second scan
  if (pendingAction != "" && millis() - pendingSince > PENDING_TIMEOUT_MS) {
    Serial.println("Pending action timed out");
    showRedThree();
    clearPending();
  }

  uint8_t uid[7];
  uint8_t uidLength;
  
  // Try to read NFC tag/card
  bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 200);

  if (!success) {
    return;
  }

  String uidString = uidToString(uid, uidLength);
  Serial.print("Scanned UID: ");
  Serial.println(uidString);
  
  // Ask backend what this UID is
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
  // Unknown / error
  Serial.println("Unexpected server response");
  Serial.println(result);
  showRedOne();
  delay(1200);
}
