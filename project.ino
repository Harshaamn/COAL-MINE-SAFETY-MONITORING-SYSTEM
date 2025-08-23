#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>

// Wi-Fi credentials
#define WIFI_SSID "harsha"
#define WIFI_PASSWORD "9***"

// Firebase credentials
#define DATABASE_URL "https://fuzzy-5eb11-default-rtdb.firebaseio.com"
#define DATABASE_SECRET "MGZE4UZ7W64dhGR94V0tLJNKoGTthea5h6B0i9Gx"

// Firebase setup
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Motor Driver Pins
const int ENA = D2;
const int IN1 = D3;
const int IN2 = D7;
const int ENB = D4;
const int IN3 = D5;
const int IN4 = D6;

// Encoder Pin
const int encoderPin = D1;

volatile unsigned long pulseCount = 0;
int distanceCM = 0;
int timeSeconds = 0;

// Encoder parameters
float wheelCircumference = 21.6; // in cm
int pulsePerRevolution = 20;

void IRAM_ATTR countPulse() {
  pulseCount++;
}

void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("Booting...");

  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(encoderPin, INPUT);

  // Motor forward direction
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);

  attachInterrupt(digitalPinToInterrupt(encoderPin), countPulse, RISING);

  // Wi-Fi setup
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\n✅ Connected to WiFi");

  // Firebase setup
  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = DATABASE_SECRET;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("✅ Firebase initialized");
}

void loop() {
  // Read inputs from Firebase
  if (Firebase.getInt(fbdo, "/botControl/distance")) {
    distanceCM = fbdo.intData();
  } else Serial.println("❌ Failed to read distance");

  if (Firebase.getInt(fbdo, "/botControl/time")) {
    timeSeconds = fbdo.intData();
  } else Serial.println("❌ Failed to read time");

  Serial.println("\n====== Firebase Inputs ======");
  Serial.print("Distance: "); Serial.println(distanceCM);
  Serial.print("Time: "); Serial.println(timeSeconds);
  Serial.println("=============================");

  if (distanceCM > 0 && timeSeconds > 0) {
    float targetSpeed = (float)distanceCM / timeSeconds;  // cm/s
    int pwm = fuzzySpeed(targetSpeed);

    analogWrite(ENA, pwm);
    analogWrite(ENB, pwm);
    Serial.print("🟢 Car Started at PWM: "); Serial.println(pwm);

    pulseCount = 0;
    unsigned long startTime = millis();
    float targetPulseCount = (float)distanceCM / (wheelCircumference / pulsePerRevolution);

    while (millis() - startTime < (timeSeconds * 1000) && pulseCount < targetPulseCount) {
      Firebase.setInt(fbdo, "/enaPulseLog/" + String(millis()), pwm);
      delay(10); // Push to Firebase every 10ms
    }

    analogWrite(ENA, 0);
    analogWrite(ENB, 0);

    if (pulseCount >= targetPulseCount) {
      Serial.println("🛑 Car stopped (pulse count reached)");
    } else {
      Serial.println("🛑 Car stopped (timeout reached)");
    }

    float finalDistance = pulseCount * wheelCircumference / pulsePerRevolution;
    float actualSpeed = finalDistance / ((millis() - startTime) / 1000.0);

    Serial.println("====== Final Report ======");
    Serial.print("Total Pulses: "); Serial.println(pulseCount);
    Serial.print("Distance Travelled: "); Serial.println(finalDistance);
    Serial.print("Actual Speed (cm/s): "); Serial.println(actualSpeed);

    Firebase.setInt(fbdo, "/finalReport/totalPulses", pulseCount);
    Firebase.setFloat(fbdo, "/finalReport/actualSpeed", actualSpeed);
    Firebase.setFloat(fbdo, "/finalReport/distanceTravelled", finalDistance);

    // Reset inputs
    Firebase.setInt(fbdo, "/botControl/distance", 0);
    Firebase.setInt(fbdo, "/botControl/time", 0);
  }

  delay(1000);
}

// ===== Fuzzy Logic for Speed → PWM Conversion =====
int fuzzySpeed(float speed) {
  float low = 0.0, medium = 0.0, high = 0.0;

  // LOW: Peak at 0, 0 at 300
  if (speed <= 300) {
    low = (300 - speed) / 300.0;
    if (low > 1) low = 1;
    if (low < 0) low = 0;
  }

  // MEDIUM: Peak at 500, 0 at 200 & 800
  if (speed >= 200 && speed <= 800) {
    if (speed <= 500)
      medium = (speed - 200) / 300.0;
    else
      medium = (800 - speed) / 300.0;
    if (medium > 1) medium = 1;
    if (medium < 0) medium = 0;
  }

  // HIGH: Peak at 1000, 0 at 600
  if (speed >= 600) {
    high = (speed - 600) / 400.0;
    if (high > 1) high = 1;
    if (high < 0) high = 0;
  }

  int pwmLow = 80;
  int pwmMedium = 170;
  int pwmHigh = 255;

  float numerator = (low * pwmLow) + (medium * pwmMedium) + (high * pwmHigh);
  float denominator = low + medium + high;

  if (denominator == 0) return 0;

  return (int)(numerator / denominator);
}
