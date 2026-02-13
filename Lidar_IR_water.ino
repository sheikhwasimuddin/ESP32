#include <Wire.h>
#include <Adafruit_VL53L0X.h>

Adafruit_VL53L0X lidar = Adafruit_VL53L0X();

// ===== NORMAL ESP32 PINS =====
#define BUZZER_PIN   25
#define IR_PIN       27
#define WATER_PIN    34   // AO Analog Pin

// I2C Pins
#define SDA_PIN 21
#define SCL_PIN 22

// Water Threshold (Adjust after testing)
int waterThreshold = 1500;

// ================= BUZZER FUNCTION =================
void beep(int onTime, int offTime) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(onTime);
  digitalWrite(BUZZER_PIN, LOW);
  delay(offTime);
}

// ================= WATER ALERT (Different Pattern) =================
void waterAlert() {
  Serial.println("💧🚨 WATER ALERT!");

  // Long-Long-Long Beep Pattern
  for (int i = 0; i < 3; i++) {
    beep(500, 300);
  }
}

// ================= IR ALERT (Fast Beep) =================
void irAlert() {
  Serial.println("⚠ IR Obstacle Detected!");
  beep(150, 150);
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(IR_PIN, INPUT);

  // Start I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // Start LiDAR
  if (!lidar.begin()) {
    Serial.println("❌ LiDAR Sensor Not Found!");
    while (1);
  }

  Serial.println("✅ ESP32 Smart Stick Started!");
  Serial.println("IR + Water + LiDAR + Buzzer Active");
}

// ================= LOOP =================
void loop() {

  // ========= 1️⃣ IR SENSOR =========
  int irValue = digitalRead(IR_PIN);

  if (irValue == LOW) {
    irAlert();   // Fast beep
  }

  // ========= 2️⃣ WATER SENSOR (Analog) =========
  int waterValue = analogRead(WATER_PIN);

  Serial.print("💧 Water Value: ");
  Serial.println(waterValue);

  if (waterValue > waterThreshold) {
    waterAlert();   // Different long beep
  }

  // ========= 3️⃣ LiDAR DISTANCE =========
  VL53L0X_RangingMeasurementData_t measure;
  lidar.rangingTest(&measure, false);

  if (measure.RangeStatus != 4) {

    int distance = measure.RangeMilliMeter;

    Serial.print("📏 Distance: ");
    Serial.print(distance);
    Serial.println(" mm");

    // LiDAR Alerts
    if (distance < 200) {
      Serial.println("🚨 Object VERY Close (<20cm)");
      beep(700, 200);   // Very long beep
    }

    else if (distance < 500) {
      Serial.println("⚠ Object Nearby (<50cm)");
      beep(350, 200);   // Medium beep
    }

    else if (distance < 1000) {
      Serial.println("🟡 Object Far (<1m)");
      beep(150, 200);   // Short beep
    }

    else {
      Serial.println("✅ Path Clear");
    }

  } else {
    Serial.println("❌ LiDAR Out of Range");
  }

  Serial.println("-----------------------------");
  delay(400);
}
