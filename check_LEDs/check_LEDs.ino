#define GREEN_LED 23
#define RED_LED 12

void setup() {
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
}

void loop() {
  // Green on
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);
  delay(1000);

  // Red on
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);
  delay(1000);

  // Both on
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, HIGH);
  delay(1000);

  // Both off
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  delay(1000);
}