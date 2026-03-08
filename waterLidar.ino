#include <WiFi.h>
#include <Wire.h>
#include <Firebase_ESP_Client.h>
#include <Adafruit_VL53L0X.h>

// Firebase Addons
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ================= WIFI =================
#define WIFI_SSID "wifi name"
#define WIFI_PASSWORD "wifipassword"

// ================= FIREBASE =================
#define API_KEY "apikey of firebase"
#define DATABASE_URL "Enter database url"

// Firebase Authentication
#define USER_EMAIL "enter email"
#define USER_PASSWORD "enter password"

// Firebase Objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ================= SENSOR =================
Adafruit_VL53L0X lidar = Adafruit_VL53L0X();

// ================= ESP32 PINS =================
#define WATER_AO_PIN 34
#define BUZZER_PIN 25

#define SDA_PIN 21
#define SCL_PIN 22

// ================= BUZZER FUNCTION =================
void beep(int onTime, int offTime)
{
  digitalWrite(BUZZER_PIN, HIGH);
  delay(onTime);
  digitalWrite(BUZZER_PIN, LOW);
  delay(offTime);
}

// ================= SEND DATA TO FIREBASE =================
void sendToFirebase(int waterValue, String waterStatus, int distance)
{
  Firebase.RTDB.setInt(&fbdo, "/Sensor/water", waterValue);
  Firebase.RTDB.setString(&fbdo, "/Sensor/waterStatus", waterStatus);
  Firebase.RTDB.setInt(&fbdo, "/Sensor/lidar", distance);

  if (fbdo.httpCode() == 200)
  {
    Serial.println("✅ Data Sent to Firebase");
  }
  else
  {
    Serial.println("❌ Firebase Error");
    Serial.println(fbdo.errorReason());
  }
}

// ================= SETUP =================
void setup()
{
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);

  Wire.begin(SDA_PIN, SCL_PIN);

  // WIFI CONNECT
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\n✅ WiFi Connected");

  // TIME SYNC
  configTime(0, 0, "pool.ntp.org");

  // FIREBASE CONFIG
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("🔥 Firebase Ready");

  // LIDAR INIT
  if (!lidar.begin())
  {
    Serial.println("❌ LiDAR Not Found");
    while (1);
  }

  Serial.println("✅ Sensors Ready");
}

// ================= LOOP =================
void loop()
{

  // ================= WATER SENSOR =================
  int waterValue = analogRead(WATER_AO_PIN);
  String waterStatus = "Dry";

  if (waterValue > 3000)
  {
    waterStatus = "Dry";
  }
  else if (waterValue > 1500)
  {
    waterStatus = "Wet";
    Serial.println("💧 Water Detected");
    beep(300,200);
  }
  else
  {
    waterStatus = "Deep Water";
    Serial.println("🌊 Deep Water Detected");
    beep(700,200);
  }

  Serial.print("Water Value: ");
  Serial.print(waterValue);
  Serial.print(" -> ");
  Serial.println(waterStatus);

  // ================= LIDAR =================
  int distance = 0;

  VL53L0X_RangingMeasurementData_t measure;

  lidar.rangingTest(&measure, false);

  if (measure.RangeStatus != 4)
  {
    distance = measure.RangeMilliMeter;

    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" mm");

    // BUZZER ALERT FOR OBSTACLE
    if (distance < 200)
      beep(700,200);
    else if (distance < 500)
      beep(350,200);
    else if (distance < 1000)
      beep(150,200);
  }
  else
  {
    Serial.println("LiDAR Out of Range");
  }

  // ================= FIREBASE =================
  sendToFirebase(waterValue, waterStatus, distance);

  Serial.println("-----------------------------");

  delay(2000);
}
