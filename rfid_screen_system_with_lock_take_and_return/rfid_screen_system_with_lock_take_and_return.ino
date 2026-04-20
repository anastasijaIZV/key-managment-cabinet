#include <WiFi.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// -----------------------------
// SCREEN PINS
// -----------------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_SDA 18
#define SCREEN_SCK 19

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// -----------------------------
// RFID READER (RDM6300)
// -----------------------------
HardwareSerial RFID(2);   // UART2
#define RFID_RX_PIN 25    // Reader TX -> ESP32 GPIO25

// -----------------------------
// LED PINS
// -----------------------------
#define GREEN_LED 23
#define RED_LED 32

// -----------------------------
// RELAY / LOCK PINS
// -----------------------------
#define RELAY_PIN 5
#define RELAY_ON  HIGH
#define RELAY_OFF LOW

// -----------------------------
// WiFi credentials
// -----------------------------
const char* ssid = "Intergalactic wi-fi";
const char* password = "theansweris42";

// -----------------------------
// Google Apps Script URL
// -----------------------------
String scriptURL = "https://script.google.com/macros/s/AKfycbwCgVd45LDD3Q-N-mTt2uKb9sZAhnYzhUHOVXR2VxBzjTWNFVHqA5EzaffC0s3cdlsr/exec";

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
const unsigned long TAG_RELEASE_MS = 400;

unsigned long pendingSince = 0;
const unsigned long PENDING_TIMEOUT_MS = 10000;

// -----------------------------
// DISPLAY HELPERS
// -----------------------------
void showMessage(String line1, String line2 = "") {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  display.setTextSize(2);
  display.println(line1);

  if (line2 != "") {
    display.setTextSize(1);
    display.println();
    display.println(line2);
  }

  display.display();
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
// LED HELPERS
// -----------------------------
void allOff() {
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
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

// Simulates the short scanner "beep" with LEDs.
// Both LEDs flash very briefly the moment a card is detected.
void scanPulse() {
  allOff();
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, HIGH);
  delay(35);
  allOff();
  delay(20);
}

void showGreenOne() {
  blinkGreen(1);
}

void showGreenThree() {
  blinkGreen(3, 90, 60);
}

void showRedOne() {
  blinkRed(1);
}

void showRedThree() {
  blinkRed(3, 90, 60);
}

void showResultThenIdle(bool ok, String line1, String line2 = "") {
  showMessage(line1, line2);
  if (ok) {
    showGreenThree();
  } else {
    showRedThree();
  }
  delay(700);  // keep result visible briefly
  clearPending();
  showMessage("Ready", "Scan card");
}

void goToIdle() {
  clearPending();
  allOff();
  showMessage("Ready", "Scan card");
}

// -----------------------------
// LOCK HELPERS
// -----------------------------
void lockInit() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF);   // keep locked at startup
}

void unlockPulse(unsigned long ms = 3000) {
  Serial.println("Unlock pulse");
  digitalWrite(RELAY_PIN, RELAY_ON);
  delay(ms);
  digitalWrite(RELAY_PIN, RELAY_OFF);
  Serial.println("Locked again");
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
  http.setTimeout(3000);  // shorter wait so UI feels less laggy

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
// RDM6300 UID READING
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

    showGreenOne();
    showMessage("OK", "User -> scan key");
    return;
  }

  if (pendingAction == "RETURN" && pendingKeyUID != "") {
    Serial.println("Completing RETURN...");
    showMessage("Scanning", "Checking user...");
    String result = processAction("RETURN", uid, pendingKeyUID);

    if (result.startsWith("OK|RETURN|")) {
      Serial.println("RETURN logged successfully");
      Serial.println(result);

      showMessage("OK", "Unlocking...");
      showGreenThree();
      unlockPulse(3000);

      clearPending();
      showMessage("Ready", "Scan card");
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
      showResultThenIdle(false, "ERROR", "Try again");
      return;
    }

    Serial.println("RETURN failed");
    Serial.println(result);
    showResultThenIdle(false, "ERROR", "Server fail");
    return;
  }

  Serial.println("Unexpected user scan");
  showResultThenIdle(false, "ERROR", "Wrong order");
}

// -----------------------------
// LOGIC: KEY SCANNED
// -----------------------------
void handleKeyFound(String uid, String name, String status, String holderUid) {
  (void)holderUid;

  if (pendingAction == "") {
    if (status != "OUT") {
      Serial.println("Key already in cabinet, cannot start RETURN");
      showResultThenIdle(false, "ERROR", "Key in cabinet");
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
    showMessage("OK", "Key -> scan user");
    return;
  }

  if (pendingAction == "TAKE" && pendingUserUID != "") {
    Serial.println("Completing TAKE...");
    showMessage("Scanning", "Checking key...");
    String result = processAction("TAKE", pendingUserUID, uid);

    if (result.startsWith("OK|TAKE|")) {
      Serial.println("TAKE logged successfully");
      Serial.println(result);

      showMessage("OK", "Unlocking...");
      showGreenThree();
      unlockPulse(3000);

      clearPending();
      showMessage("Ready", "Scan card");
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
      showResultThenIdle(false, "ERROR", "Try again");
      return;
    }

    Serial.println("TAKE failed");
    Serial.println(result);
    showResultThenIdle(false, "ERROR", "Server fail");
    return;
  }

  Serial.println("Unexpected key scan");
  showResultThenIdle(false, "ERROR", "Wrong order");
}

// -----------------------------
// SETUP
// -----------------------------
void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  allOff();

  lockInit();

  Wire.begin(SCREEN_SDA, SCREEN_SCK);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed");
    while (true) {
      delay(10);
    }
  }

  showMessage("Booting", "WiFi...");

  RFID.begin(9600, SERIAL_8N1, RFID_RX_PIN, -1);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());

  clearPending();
  Serial.println("RDM6300 ready");
  Serial.println("Scan user first for TAKE, or key first for RETURN");

  goToIdle();
}

// -----------------------------
// LOOP
// -----------------------------
void loop() {
  if (pendingAction != "" && millis() - pendingSince > PENDING_TIMEOUT_MS) {
    Serial.println("Pending action timed out");
    showResultThenIdle(false, "ERROR", "Timeout");
    return;
  }

  String rawUid = readRdm6300UidRaw();

  if (rawUid != "") {
    lastRawReadTime = millis();

    if (rawUid == currentPresentedUid) {
      return;
    }

    currentPresentedUid = rawUid;
    String uidString = rawUid;

    Serial.print("Scanned UID: ");
    Serial.println(uidString);

    // Immediate feedback: LED pulse + screen update before server call
    scanPulse();
    showMessage("Scanning", uidString);

    String result = classifyUid(uidString);

    if (
      result == "NOT_FOUND" ||
      result == "USER_INACTIVE" ||
      result == "DUPLICATE_UID" ||
      result == "ERROR"
    ) {
      Serial.println("Classify failed / access denied");
      Serial.println(result);
      showResultThenIdle(false, "ERROR", "Unknown card");
      return;
    }

    if (result.startsWith("USER_FOUND|")) {
      String foundUid = getPart(result, 1);
      String userName = getPart(result, 2);
      handleUserFound(foundUid, userName);
      return;
    }

    if (result.startsWith("KEY_FOUND|")) {
      String foundUid = getPart(result, 1);
      String keyName = getPart(result, 2);
      String status = getPart(result, 3);
      String holderUid = getPart(result, 4);
      handleKeyFound(foundUid, keyName, status, holderUid);
      return;
    }

    Serial.println("Unexpected server response");
    Serial.println(result);
    showResultThenIdle(false, "ERROR", "Bad reply");
    return;
  }

  if (currentPresentedUid != "" && millis() - lastRawReadTime > TAG_RELEASE_MS) {
    currentPresentedUid = "";
  }
}
