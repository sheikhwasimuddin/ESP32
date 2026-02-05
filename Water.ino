#define WATER_PIN 34

void setup() {
  Serial.begin(115200);
}

void loop() {
  int waterValue = analogRead(WATER_PIN);

  Serial.print("Water Value: ");
  Serial.println(waterValue);

  delay(1000);
}
