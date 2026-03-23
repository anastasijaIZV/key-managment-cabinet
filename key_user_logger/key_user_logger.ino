#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_PN532.h>

#define SDA_PIN 21
#define SCL_PIN 22

Adafruit_PN532 nfc(-1, -1);

const char* ssid = "Intergalactic wi-fi";
const char* password = "theansweris42";
String scriptURL = "https://script.google.com/macros/s/AKfycbxl0Eu9Jq56P7El38Uv9RTevcxsWZuTaiHma_en8DilFnrhRQxrahxN-Fq0brLDdglg/exec";

// -------------------------
// SETTINGS
// -------------------------
const unsigned long SESSION_TIMEOUT_MS = 15000;
const unsigned long DUPLICATE_IGNORE_MS = 1200;

// -------------------------
// STATE MACHINE
// -------------------------
enum SystemState {
  IDLE,
  TAKE_MODE,
  RETURN_MODE
};

SystemState state = IDLE;

// -------------------------
// SESSION VARIABLES
// -------------------------
String currentUserUID = "";
String currentKeyUID = "";
unsigned long sessionStartTime = 0;

String lastScannedUID = "";
unsigned long lastScanTime = 0;

struct UserRecord {
  const char* uid;
  const char* name;
  bool active;
};

struct KeyRecord {
  const char* uid;
  const char* name;
};

UserRecord users[] = {
  {"C3B71807", "Admin", true},
  {"198804AF", "Bob", true}
};

KeyRecord keys[] = {
  {"836D2607", "Admin key"},
  {"91FF0021", "Storage Key"}
};

const int USER_COUNT = sizeof(users) / sizeof(users[0]);
const int KEY_COUNT  = sizeof(keys) / sizeof(keys[0]);

void showMessage(String msg) {
  Serial.println(msg);
}

void resetSession() {
  currentUserUID = "";
  currentKeyUID = "";
  sessionStartTime = 0;
  state = IDLE;
  showMessage("IDLE -> Scan card to TAKE, or key to RETURN");
}

void startSession() {
  sessionStartTime = millis();
}

void refreshSession() {
  sessionStartTime = millis();
}

bool isSessionTimedOut() {
  if (state == IDLE) return false;
  return (millis() - sessionStartTime > SESSION_TIMEOUT_MS);
}

String uidToString(uint8_t *uid, uint8_t uidLength) {
  String uidStr = "";

  for (uint8_t i = 0; i < uidLength; i++) {
    if (uid[i] < 0x10) uidStr += "0";
    uidStr += String(uid[i], HEX);
  }

  uidStr.toUpperCase();
  return uidStr;
}

bool isDuplicateScan(const String& uid) {
  unsigned long now = millis();

  if (uid == lastScannedUID && (now - lastScanTime) < DUPLICATE_IGNORE_MS) {
    return true;
  }

  lastScannedUID = uid;
  lastScanTime = now;
  return false;
}

bool isKnownUser(const String& uid) {
  for (int i = 0; i < USER_COUNT; i++) {
    if (uid == users[i].uid && users[i].active) return true;
  }
  return false;
}

bool isKnownKey(const String& uid) {
  for (int i = 0; i < KEY_COUNT; i++) {
    if (uid == keys[i].uid) return true;
  }
  return false;
}

String getUserName(const String& uid) {
  for (int i = 0; i < USER_COUNT; i++) {
    if (uid == users[i].uid) return String(users[i].name);
  }
  return "Unknown User";
}

String getKeyName(const String& uid) {
  for (int i = 0; i < KEY_COUNT; i++) {
    if (uid == keys[i].uid) return String(keys[i].name);
  }
  return "Unknown Key";
}

bool sendToGoogle(String action, String userUID, String keyUID) {
  if (WiFi.status() != WL_CONNECTED) {
    showMessage("WiFi not connected");
    return false;
  }

  HTTPClient http;

  String url = scriptURL
             + "?action=" + action
             + "&userUID=" + userUID
             + "&keyUID=" + keyUID;

  showMessage("Sending -> " + url);

  http.begin(url);
  int httpCode = http.GET();

  Serial.print("HTTP Response: ");
  Serial.println(httpCode);

  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("Server: " + response);
  }

  http.end();
  return (httpCode > 0);
}

void handleIdleScan(const String& uid) {
  if (isKnownUser(uid)) {
    currentUserUID = uid;
    state = TAKE_MODE;
    startSession();

    showMessage("TAKE MODE");
    showMessage("User: " + getUserName(uid));
    showMessage("Scan key(s) to take");
    return;
  }

  if (isKnownKey(uid)) {
    currentKeyUID = uid;
    state = RETURN_MODE;
    startSession();

    showMessage("RETURN MODE");
    showMessage("Key: " + getKeyName(uid));
    showMessage("Scan user card to return");
    return;
  }

  showMessage("Unknown UID in IDLE: " + uid);
}

void handleTakeModeScan(const String& uid) {
  if (isKnownKey(uid)) {
    showMessage(getUserName(currentUserUID) + " -> TAKE -> " + getKeyName(uid));

    bool ok = sendToGoogle("TAKE", currentUserUID, uid);

    if (ok) {
      showMessage("TAKE logged");
    } else {
      showMessage("Failed to log TAKE");
    }

    refreshSession();
    showMessage("Scan another key or same user card to finish");
    return;
  }

  if (isKnownUser(uid)) {
    if (uid == currentUserUID) {
      showMessage("Ending TAKE session for " + getUserName(uid));
      resetSession();
    } else {
      showMessage("Different user scanned during TAKE mode -> rejected");
      refreshSession();
    }
    return;
  }

  showMessage("Unknown UID in TAKE mode");
  refreshSession();
}

void handleReturnModeScan(const String& uid) {
  if (isKnownUser(uid)) {
    showMessage(getUserName(uid) + " -> RETURN -> " + getKeyName(currentKeyUID));

    bool ok = sendToGoogle("RETURN", uid, currentKeyUID);

    if (ok) {
      showMessage("RETURN logged");
    } else {
      showMessage("Failed to log RETURN");
    }

    resetSession();
    return;
  }

  if (isKnownKey(uid)) {
    showMessage("Still waiting for user card in RETURN mode");
    refreshSession();
    return;
  }

  showMessage("Unknown UID in RETURN mode");
  refreshSession();
}

void processScan(const String& uid) {
  if (isDuplicateScan(uid)) {
    showMessage("Duplicate scan ignored: " + uid);
    return;
  }

  showMessage("Scanned UID: " + uid);

  switch (state) {
    case IDLE:
      handleIdleScan(uid);
      break;

    case TAKE_MODE:
      handleTakeModeScan(uid);
      break;

    case RETURN_MODE:
      handleReturnModeScan(uid);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");

  nfc.begin();
  if (!nfc.getFirmwareVersion()) {
    Serial.println("PN532 not found");
    while (1);
  }

  nfc.SAMConfig();
  resetSession();
}

void loop() {
  if (isSessionTimedOut()) {
    showMessage("Session timeout");
    resetSession();
  }

  uint8_t uid[7];
  uint8_t uidLength;

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    String uidStr = uidToString(uid, uidLength);
    processScan(uidStr);
    delay(250);
  }
}
