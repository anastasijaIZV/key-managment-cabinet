#include <WiFi.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void showMessage(String line1, String line2 = "") {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.setTextColor(WHITE);

  display.println(line1);

  if (line2 != "") {
    display.setTextSize(1);
    display.println(line2);
  }

  display.display();
}

// -----------------------------
// RFID READER (RDM6300)
// -----------------------------
HardwareSerial RFID(2);   // UART2
#define RFID_RX_PIN 25    // Reader TX -> ESP32 GPIO14

// -----------------------------
// LED pins
// -----------------------------
#define GREEN_LED 23
#define RED_LED 32

// -----------------------------
// WiFi credentials
// -----------------------------
const char* ssid = "Intergalactic wi-fi";
const char* password = "theansweris42";

// -----------------------------
// Google Apps Script URL
// -----------------------------
String scriptURL = "https://script.google.com/macros/s/AKfycbz41gmbVFUSphWrWmNBEIWWz_UMrxw2a0EXBWxRLJdLolRXIhlEvasvqPbezLdTb_Au/exec";

// -----------------------------
// Pending transaction state
// -----------------------------
String pendingAction = "";   // TAKE or RETURN
String pendingUserUID = "";
String pendingUserName = "";
String pendingKeyUID = "";
String pendingKeyName = "";

String currentPresentedUid = "";
unsigned long lastRawReadTime = 0;
const unsigned long TAG_RELEASE_MS = 400;   // tag considered removed after no reads for 400 ms

// Timestamp for timeout
unsigned long pendingSince = 0;
const unsigned long PENDING_TIMEOUT_MS = 10000;

// -----------------------------
// LED HELPERS
// -----------------------------
void allOff() {
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
}

void goToIdle() {
  clearPending();
  delay(250);
  showMessage("Ready", "Scan card");
}

void blinkGreen(int count, int onMs = 70, int offMs = 50) {
  for (int i = 0; i < count; i++) {
    allOff();
    digitalWrite(GREEN_LED, HIGH);
    delay(onMs);
    digitalWrite(GREEN_LED, LOW);
    delay(offMs);
  }
}

void blinkRed(int count, int onMs = 70, int offMs = 50) {
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

// -----------------------------
// RESET CURRENT TRANSACTION
// -----------------------------
void clearPending() {
  pendingAction = "";
  pendingUserUID = "";
  pendingUserName = "";
  pendingKeyUID = "";
  pendingKeyName = "";
  pendingSince = 0;
}

// -----------------------------
// HTTP COMMUNICATION
// -----------------------------
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

// -----------------------------
// RESPONSE PARSING
// -----------------------------
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
// RDM6300 UID READING format is usually
// 0x02 + 10 ASCII HEX chars + 2 ASCII HEX checksum + 0x03
// Keeping the first 10 hex chars as the tag UID.
// -----------------------------
String readRdm6300UidRaw() {
  static String buffer = "";

  while (RFID.available()) {
    char c = RFID.read();

    if (c == 0x02) {
      buffer = "";
    } else if (c == 0x03) {
      String frame = buffer;
      buffer = "";

      frame.trim();

      if (frame.length() >= 10) {
        String uid = frame.substring(0, 10);
        uid.toUpperCase();
        return uid;
      }
    } else {
      if (isPrintable(c)) {
        buffer += c;
      }
    }
  }

  return "";
}

// -----------------------------
// LOGIC: USER SCANNED
// -----------------------------
void handleUserFound(String uid, String name) {
  if (pendingAction == "") {
    pendingAction = "TAKE";
    pendingUserUID = uid;
    pendingUserName = name;
    pendingSince = millis();

    Serial.println("User scanned first -> pending TAKE");
    Serial.print("User: ");
    Serial.println(name);

    showMessage("User OK", "Scan key");
    showGreenOne();
    return;
  }

  if (pendingAction == "RETURN" && pendingKeyUID != "") {
    Serial.println("Completing RETURN...");
    String result = processAction("RETURN", uid, pendingKeyUID);

    if (result.startsWith("OK|RETURN|")) {
      Serial.println("RETURN logged successfully");
      Serial.println(result);
      showMessage("SUCCESS", "Done");
      showGreenThree();
      goToIdle();
      return;
    }

    if (
      result == "KEY_ALREADY_IN" ||
      result == "USER_NOT_FOUND" ||
      result == "USER_INACTIVE" ||
      result == "KEY_NOT_FOUND"
    ) {
      Serial.println("RETURN denied");
      Serial.println(result);
      showMessage("ERROR", "Try again");
      showRedOne();
      goToIdle();
      return;
    }

    Serial.println("RETURN failed");
    Serial.println(result);
    showMessage("ERROR", "Try again");
    showRedOne();
    goToIdle();
    return;
  }

  Serial.println("Unexpected user scan");
  showMessage("ERROR", "Try again");
  showRedOne();
  goToIdle();
  return;
}

// -----------------------------
// LOGIC: KEY SCANNED
// -----------------------------
void handleKeyFound(String uid, String name, String status, String holderUid) {
  if (pendingAction == "") {
    if (status != "OUT") {
      Serial.println("Key already in cabinet, cannot start RETURN");
      showMessage("ERROR", "Key in cabinet");
      showRedOne();
      goToIdle();
      return;
    }

    pendingAction = "RETURN";
    pendingKeyUID = uid;
    pendingKeyName = name;
    pendingSince = millis();

    Serial.println("Key scanned first -> pending RETURN");
    Serial.print("Key: ");
    Serial.println(name);

    showMessage("Key OK", "Scan user");
    showGreenOne();
    return;
  }

  if (pendingAction == "TAKE" && pendingUserUID != "") {
    Serial.println("Completing TAKE...");
    String result = processAction("TAKE", pendingUserUID, uid);

    if (result.startsWith("OK|TAKE|")) {
      Serial.println("TAKE logged successfully");
      Serial.println(result);
      showMessage("SUCCESS", "Done");
      showGreenThree();
      goToIdle();
      return;
    }

    if (
      result == "KEY_NOT_AVAILABLE" ||
      result == "USER_NOT_FOUND" ||
      result == "USER_INACTIVE" ||
      result == "KEY_NOT_FOUND"
    ) {
      Serial.println("TAKE denied");
      Serial.println(result);
      showMessage("ERROR", "Try again");
      showRedOne();
      goToIdle();
      return;
    }

    Serial.println("TAKE failed");
    Serial.println(result);
    showMessage("ERROR", "Try again");
    showRedOne();
    goToIdle();
    return;
  }

  Serial.println("Unexpected key scan");
  showMessage("ERROR", "Try again");
  showRedOne();
  goToIdle();
  return;
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

  // Start RFID reader
  RFID.begin(9600, SERIAL_8N1, RFID_RX_PIN, -1);

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

  clearPending();

  Serial.println("RDM6300 ready");
  Serial.println("Scan user first for TAKE, or key first for RETURN");

  Wire.begin(18, 19);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed");
    while (true);
  }

  display.clearDisplay();
  showMessage("Ready", "Scan card");
}

// -----------------------------
// LOOP
// -----------------------------
void loop() {
  if (pendingAction != "" && millis() - pendingSince > PENDING_TIMEOUT_MS) {
    Serial.println("Pending action timed out");
    showMessage("Timeout", "Start again");
    showRedThree();
    goToIdle();
    return;
}

  String rawUid = readRdm6300UidRaw();

// Track whether the currently presented tag is still there
if (rawUid != "") {
  lastRawReadTime = millis();

  // Same tag still being held in front of reader -> ignore
  if (rawUid == currentPresentedUid) {
    return;
  }

  // New tag appeared
  currentPresentedUid = rawUid;
  String uidString = rawUid;

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
    goToIdle();
    return;
  }

  if (result.startsWith("USER_FOUND|")) {
    String foundUid = getPart(result, 1);
    String userName = getPart(result, 2);
    handleUserFound(foundUid, userName);
    delay(150);
    return;
  }

  if (result.startsWith("KEY_FOUND|")) {
    String foundUid = getPart(result, 1);
    String keyName = getPart(result, 2);
    String status = getPart(result, 3);
    String holderUid = getPart(result, 4);
    handleKeyFound(foundUid, keyName, status, holderUid);
    delay(150);
    return;
  }

  Serial.println("Unexpected server response");
  Serial.println(result);
  showRedOne();
  delay(150);
  return;
}

// If nothing has been read for a short time, consider the tag removed
if (currentPresentedUid != "" && millis() - lastRawReadTime > TAG_RELEASE_MS) {
  currentPresentedUid = "";
}
}