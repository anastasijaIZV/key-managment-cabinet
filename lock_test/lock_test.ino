#define RELAY_PIN 26

// Most relay modules are active LOW.
// If yours behaves backwards, swap these two values.
#define RELAY_ON  HIGH
#define RELAY_OFF LOW

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);

  // Keep relay idle at startup
  digitalWrite(RELAY_PIN, RELAY_OFF);

  Serial.println("Lock test ready.");
  Serial.println("Type anything in Serial Monitor and press Enter.");
  Serial.println("Relay will activate for 3 seconds.");
}

void loop() {
  if (Serial.available() > 0) {
    while (Serial.available() > 0) {
      Serial.read();
    }

    Serial.println("Unlock pulse");
    digitalWrite(RELAY_PIN, RELAY_ON);

    delay(3000);

    digitalWrite(RELAY_PIN, RELAY_OFF);
    Serial.println("Locked again");
  }
}