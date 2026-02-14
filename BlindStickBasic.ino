#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

// ================== OBJECTS ==================
Adafruit_VL53L0X lidar = Adafruit_VL53L0X();
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// ================== ESP32 PINS ==================
#define BUZZER_PIN     25
#define IR_PIN         27
#define WATER_DO_PIN   33   // ✅ Water Sensor DO Pin

#define SDA_PIN 21
#define SCL_PIN 22

// ================== FALL THRESHOLDS ==================
float freeFallThreshold = 5.0;   // Below this = Free Fall
float impactThreshold   = 25.0;  // Above this = Impact

// ================== BUZZER FUNCTION ==================
void beep(int onTime, int offTime) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(onTime);
  digitalWrite(BUZZER_PIN, LOW);
  delay(offTime);
}

// ================== ALERT PATTERNS ==================

void waterAlert() {
  Serial.println("💧🚨 WATER DETECTED!");
  for (int i = 0; i < 3; i++) {
    beep(500, 300);   // Long beeps
  }
}

void irAlert() {
  Serial.println("⚠ IR Obstacle Detected!");
  beep(150, 150);     // Fast beep
}

void fallAlert() {
  Serial.println("🚨🚨 FALL DETECTED!");
  for (int i = 0; i < 5; i++) {
    beep(200, 200);   // Emergency beeps
  }
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(IR_PIN, INPUT);
  pinMode(WATER_DO_PIN, INPUT);

  // Start I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // Start LiDAR
  if (!lidar.begin()) {
    Serial.println("❌ LiDAR Sensor Not Found!");
    while (1);
  }

  // Start ADXL345
  if (!accel.begin()) {
    Serial.println("❌ ADXL345 Not Found!");
    while (1);
  }

  Serial.println("✅ ESP32 Smart Stick Started!");
  Serial.println("IR + Water(DO) + LiDAR + Fall Detection Active");
}

// ================== LOOP ==================
void loop() {

  // ========= 1️⃣ IR SENSOR =========
  int irValue = digitalRead(IR_PIN);

  if (irValue == LOW) {
    irAlert();
  }

  // ========= 2️⃣ WATER SENSOR (DO DIGITAL) =========
  int waterState = digitalRead(WATER_DO_PIN);

  Serial.print("💧 Water DO State: ");
  Serial.println(waterState);

  // Most modules: LOW means water detected
  if (waterState == LOW) {
    waterAlert();
  }

  // ========= 3️⃣ LiDAR DISTANCE =========
  VL53L0X_RangingMeasurementData_t measure;
  lidar.rangingTest(&measure, false);

  if (measure.RangeStatus != 4) {

    int distance = measure.RangeMilliMeter;

    Serial.print("📏 Distance: ");
    Serial.print(distance);
    Serial.println(" mm");

    if (distance < 200) {
      Serial.println("🚨 Object VERY Close (<20cm)");
      beep(700, 200);
    }
    else if (distance < 500) {
      Serial.println("⚠ Object Nearby (<50cm)");
      beep(350, 200);
    }
    else if (distance < 1000) {
      Serial.println("🟡 Object Far (<1m)");
      beep(150, 200);
    }
    else {
      Serial.println("✅ Path Clear");
    }

  } else {
    Serial.println("❌ LiDAR Out of Range");
  }

  // ========= 4️⃣ FALL DETECTION (FREE FALL + IMPACT) =========

  sensors_event_t event;
  accel.getEvent(&event);

  float x = event.acceleration.x;
  float y = event.acceleration.y;
  float z = event.acceleration.z;

  float magnitude = sqrt(x * x + y * y + z * z);

  Serial.print("📉 Accel Magnitude: ");
  Serial.println(magnitude);

  // Step 1: Free Fall
  if (magnitude < freeFallThreshold) {

    Serial.println("🟡 Possible Free Fall Detected...");

    delay(300); // Wait for impact

    // Step 2: Impact Check
    accel.getEvent(&event);

    float impactMag = sqrt(
      event.acceleration.x * event.acceleration.x +
      event.acceleration.y * event.acceleration.y +
      event.acceleration.z * event.acceleration.z
    );

    Serial.print("💥 Impact Magnitude: ");
    Serial.println(impactMag);

    if (impactMag > impactThreshold) {
      fallAlert();
      delay(3000); // Prevent repeat
    }
  }

  Serial.println("-----------------------------");
  delay(400);
}
