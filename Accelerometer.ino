#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

#define BUZZER_PIN 25   // Optional buzzer pin

void setup() {
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);

  if (!accel.begin()) {
    Serial.println("❌ ADXL345 not detected!");
    while (1);
  }

  Serial.println("✅ ADXL345 Connected!");
}

void loop() {
  sensors_event_t event;
  accel.getEvent(&event);

  // Read acceleration values
  float x = event.acceleration.x;
  float y = event.acceleration.y;
  float z = event.acceleration.z;

  // Calculate magnitude
  float magnitude = sqrt(x * x + y * y + z * z);

  Serial.print("Accel Magnitude: ");
  Serial.println(magnitude);

  // Fall detection threshold
  if (magnitude > 10) {
    Serial.println("🚨 FALL DETECTED!");

    // Buzzer alert
    digitalWrite(BUZZER_PIN, HIGH);
    delay(1000);
    digitalWrite(BUZZER_PIN, LOW);

    delay(2000); // Prevent repeated triggers
  }

  delay(200);
}
