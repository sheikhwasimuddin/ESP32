#include <WiFi.h>
#include <Wire.h>
#include <Firebase_ESP_Client.h>

#include <Adafruit_VL53L0X.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

// Firebase Addons
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ================= WIFI =================
#define WIFI_SSID "enter you wifi name"
#define WIFI_PASSWORD "enter your password"

// ================= FIREBASE =================
#define API_KEY "apikey of firebase"
#define DATABASE_URL "enter your realtime database url"

// Firebase Authentication User
#define USER_EMAIL "enter your email"
#define USER_PASSWORD "enter your password"

// Firebase Objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ================= SENSOR OBJECTS =================
Adafruit_VL53L0X lidar = Adafruit_VL53L0X();
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// ================= ESP32 PINS =================
#define BUZZER_PIN     25
#define IR_PIN         27
#define WATER_AO_PIN   34

#define SDA_PIN 21
#define SCL_PIN 22

// ================= FALL THRESHOLDS =================
float freeFallThreshold = 5.0;
float impactThreshold   = 25.0;

// ================= BUZZER FUNCTION =================
void beep(int onTime, int offTime) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(onTime);
  digitalWrite(BUZZER_PIN, LOW);
  delay(offTime);
}

// ================= SEND TO FIREBASE RTDB =================
void sendToFirebase(bool fall, int waterValue, String waterStatus,
                    bool obstacle, int distance) {

  Firebase.RTDB.setBool(&fbdo, "/Sensor/fallDetected", fall);

  Firebase.RTDB.setInt(&fbdo, "/Sensor/water", waterValue);
  Firebase.RTDB.setString(&fbdo, "/Sensor/waterStatus", waterStatus);

  Firebase.RTDB.setBool(&fbdo, "/Sensor/stairpitholeDetected", obstacle);
  Firebase.RTDB.setInt(&fbdo, "/Sensor/lidar", distance);

  if (fbdo.httpCode() == 200) {
    Serial.println("✅ Data Sent to Firebase!");
  } else {
    Serial.println("❌ Firebase Error:");
    Serial.println(fbdo.errorReason());
  }
}

// ================= SETUP =================
void setup() {

  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(IR_PIN, INPUT);

  Wire.begin(SDA_PIN, SCL_PIN);

  // ================= WIFI CONNECT =================
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\n✅ WiFi Connected!");
  Serial.println(WiFi.localIP());

  // ================= TIME SYNC =================
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Syncing Time");
  time_t now = time(nullptr);

  while (now < 100000) {
    Serial.print(".");
    delay(500);
    now = time(nullptr);
  }

  Serial.println("\n✅ Time Synced!");

  // ================= FIREBASE CONFIG =================
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("🔥 Firebase Ready!");

  // ================= SENSOR INIT =================
  if (!lidar.begin()) {
    Serial.println("❌ LiDAR Not Found!");
    while (1);
  }

  if (!accel.begin()) {
    Serial.println("❌ ADXL345 Not Found!");
    while (1);
  }

  Serial.println("✅ Smart Blind Stick Started Successfully!");
}

// ================= LOOP =================
void loop() {

  // =====================================================
  // ✅ WATER SENSOR (ANALOG)
  // =====================================================
  int total = 0;
  for (int i = 0; i < 10; i++) {
    total += analogRead(WATER_AO_PIN);
    delay(5);
  }

  int waterValue = total / 10;
  String waterStatus = "Dry";

  if (waterValue > 3000) {
    waterStatus = "Dry";
  }
  else if (waterValue > 1500) {
    waterStatus = "Wet";
    Serial.println("💧 Water Detected (Wet)");
    beep(300, 200);
  }
  else {
    waterStatus = "Deep Water";
    Serial.println("🌊 Deep Water Detected!");
    beep(700, 200);
  }

  Serial.print("💧 Water Value: ");
  Serial.print(waterValue);
  Serial.print(" → ");
  Serial.println(waterStatus);

  // =====================================================
  // ✅ IR OBSTACLE SENSOR
  // =====================================================
  bool obstacleDetected = (digitalRead(IR_PIN) == LOW);

  if (obstacleDetected) {
    Serial.println("⚠ IR Obstacle Detected!");
    beep(150, 150);
  }

  // =====================================================
  // ✅ LiDAR DISTANCE SENSOR
  // =====================================================
  int distance = 0;
  VL53L0X_RangingMeasurementData_t measure;
  lidar.rangingTest(&measure, false);

  if (measure.RangeStatus != 4) {

    distance = measure.RangeMilliMeter;

    Serial.print("📏 Distance: ");
    Serial.print(distance);
    Serial.println(" mm");

    if (distance < 200) beep(700, 200);
    else if (distance < 500) beep(350, 200);
    else if (distance < 1000) beep(150, 200);

  } else {
    Serial.println("❌ LiDAR Out of Range");
  }

  // =====================================================
  // ✅ ACCELEROMETER READINGS + FALL DETECTION
  // =====================================================
  bool fallDetected = false;

  sensors_event_t event;
  accel.getEvent(&event);

  float magnitude = sqrt(
    event.acceleration.x * event.acceleration.x +
    event.acceleration.y * event.acceleration.y +
    event.acceleration.z * event.acceleration.z
  );

  // ✅ PRINT LIVE ACCELEROMETER VALUES
  Serial.println("📌 Accelerometer Readings:");
  Serial.print("X: "); Serial.print(event.acceleration.x); Serial.print("  | ");
  Serial.print("Y: "); Serial.print(event.acceleration.y); Serial.print("  | ");
  Serial.print("Z: "); Serial.println(event.acceleration.z);

  Serial.print("📉 Magnitude: ");
  Serial.println(magnitude);
  Serial.println("-----------------------------");

  // ========= FALL DETECTION =========
  if (magnitude < freeFallThreshold) {

    Serial.println("⚠ Possible Free Fall Detected...");
    delay(300);

    accel.getEvent(&event);

    float impactMag = sqrt(
      event.acceleration.x * event.acceleration.x +
      event.acceleration.y * event.acceleration.y +
      event.acceleration.z * event.acceleration.z
    );

    Serial.print("💥 Impact Magnitude: ");
    Serial.println(impactMag);

    if (impactMag > impactThreshold) {
      fallDetected = true;
      Serial.println("🚨 FALL DETECTED!");
      beep(200, 200);
      delay(1000);
    }
  }

  // =====================================================
  // ✅ SEND DATA TO FIREBASE
  // =====================================================
  sendToFirebase(fallDetected, waterValue, waterStatus,
                 obstacleDetected, distance);

  Serial.println("====================================");

  delay(1000);
}
