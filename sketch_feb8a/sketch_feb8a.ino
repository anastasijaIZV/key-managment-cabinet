#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

const char* SSID = "YOUR_WIFI_NAME";
const char* PASSWORD = "YOUR_WIFI_PASSWORD";
const char* WEB_APP_URL = "YOUR_GOOGLE_APPS_SCRIPT_URL";

void setup() {
  Serial.begin(115200);
  delay(500);

  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected");

  WiFiClientSecure client;
  client.setInsecure();  // 🔑 critical

  HTTPClient https;
  Serial.println("Starting HTTPS...");

  if (!https.begin(client, URL)) {
    Serial.println("❌ https.begin() failed");
    return;
  }

  https.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpCode = https.POST("device=esp32&event=TLS_TEST");

  Serial.print("HTTP status: ");
  Serial.println(httpCode);

  if (httpCode > 0) {
    Serial.println(https.getString());
  }

  https.end();
}

void loop() {}
