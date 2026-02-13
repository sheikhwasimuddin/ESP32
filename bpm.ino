#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pins and Threshold
const int pulsePin = 34;
const int threshold = 2000;

const int buttonPin = 5;
int buttonState = HIGH;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;
int buttonPressCount = 0;

// Timing and BPM Calculation
unsigned long startTime;
const unsigned long measurementDuration = 10000;

int Signal;
boolean Pulse = false;
unsigned long lastBeatTime = 0;
unsigned long sampleCounter = 0;
unsigned long lastTime = 0;
int rate[10];
int rateSpot = 0;
int beatCount = 0;
bool measuring = false;

int finalBPM = 0;
int lastBPM = 0;

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();

  pinMode(pulsePin, INPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  for (int i = 0; i < 10; i++) rate[i] = 600;

  lcd.setCursor(0, 0);
  lcd.print("Place finger...");
  delay(2000);

  // Countdown before measurement
  for (int i = 10; i >= 1; i--) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Start in:");
    lcd.setCursor(8, 1);
    lcd.print(i);
    delay(1000);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Measuring...");

  measuring = true;
  lastTime = millis();
  startTime = millis();
  sampleCounter = 0;
}

void loop() {
  // --- Button Debounce ---
  int reading = digitalRead(buttonPin);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        buttonPressCount++;
        handleButtonPress();
      }
    }
  }
  lastButtonState = reading;

  if (!measuring) return;

  unsigned long now = millis();
  Signal = analogRead(pulsePin);
  sampleCounter += (now - lastTime);
  lastTime = now;

  // Detect heartbeat
  if (Signal > threshold && Pulse == false) {
    Pulse = true;
    unsigned long IBI = sampleCounter - lastBeatTime;
    lastBeatTime = sampleCounter;

    if (IBI > 300 && IBI < 2000) {
      rate[rateSpot] = IBI;
      rateSpot = (rateSpot + 1) % 10;
      beatCount++;
    }
  }

  if (Signal < threshold && Pulse == true) {
    Pulse = false;
  }

  // Stop measuring after duration
  if (now - startTime >= measurementDuration) {
    measuring = false;
    lcd.clear();

    if (beatCount < 2) {
      lcd.setCursor(0, 0);
      lcd.print("No pulse found");
      Serial.println("No pulse found.");
    } else {
      int validCount = 0;
      long sumIBI = 0;
      for (int i = 0; i < 10; i++) {
        if (rate[i] > 0) {
          sumIBI += rate[i];
          validCount++;
        }
      }

      int avgIBI = sumIBI / validCount;
      lastBPM = finalBPM;  // Save previous value
      finalBPM = 60000 / avgIBI;

      lcd.setCursor(0, 0);
      lcd.print("Heart Rate:");
      lcd.setCursor(0, 1);
      lcd.print(finalBPM);
      lcd.print(" BPM");

      Serial.print("Final BPM: ");
      Serial.println(finalBPM);
    }
  }

  delay(20); // slight delay for stability
}

void handleButtonPress() {
  if (buttonPressCount == 1) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Last BPM:");
    lcd.setCursor(0, 1);
    if (lastBPM == 0) {
      lcd.print("No data");
    } else {
      lcd.print(lastBPM);
      lcd.print(" BPM");
    }
  } else if (buttonPressCount == 2) {
    buttonPressCount = 0;
    // Restart measurement
    for (int i = 0; i < 10; i++) rate[i] = 600;
    beatCount = 0;
    Pulse = false;
    sampleCounter = 0;
    lastBeatTime = 0;
    lastTime = millis();
    startTime = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Measuring...");
    measuring = true;
  }
}
