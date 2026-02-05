#define IR_PIN 15

void setup() {
  pinMode(IR_PIN, INPUT);
  Serial.begin(115200);
}

void loop() {
  if (digitalRead(IR_PIN) == LOW) {
    Serial.println("Kuch to aaya hai biche me");
  } else {
    Serial.println("aaghe chalo");
  }
  delay(500);
}
