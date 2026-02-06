#define BUZZER_PIN 12
#define IR_PIN     13
#define WATER_PIN  14   // DO pin

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(IR_PIN, INPUT_PULLUP);     
  pinMode(WATER_PIN, INPUT_PULLUP);  

  digitalWrite(BUZZER_PIN, LOW);

  Serial.begin(115200);
  Serial.println("Blind Stick Ready");
}

void obstacleBeep() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(120);
  digitalWrite(BUZZER_PIN, LOW);
  delay(120);
}

void loop() {
  int irRaw = digitalRead(IR_PIN);
  int waterRaw = digitalRead(WATER_PIN);

  bool obstacleDetected = (irRaw == LOW);    // LOW = obstacle
  bool waterDetected    = (waterRaw == LOW); // LOW = water

  // Serial debug (TRUTH, not confusion)
  Serial.print("Obstacle: ");
  Serial.print(obstacleDetected ? "YES" : "NO");

  Serial.print(" | Water: ");
  Serial.println(waterDetected ? "YES" : "NO");

  if (waterDetected) {
    digitalWrite(BUZZER_PIN, HIGH);   // continuous sound
  }
  else if (obstacleDetected) {
    obstacleBeep();                  // beep-beep
  }
  else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  delay(50);
}
