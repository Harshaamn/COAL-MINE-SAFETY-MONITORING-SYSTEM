#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <DHT.h>

// Firebase config
#define FIREBASE_HOST "coal-project-5e6db-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "nDwKqzGeQJjcLP98Kq3xt8Ib4ascTwQ4mXusBqi"

// Wi-Fi credentials
#define WIFI_SSID "harsha"
#define WIFI_PASSWORD "93****"

// DHT Sensor setup
#define DHTPIN D2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

FirebaseData firebaseData;
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;

// Buzzer setup
#define BUZZER_PIN D4

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Setup buzzer pin
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to Wi-Fi");

  // Configure Firebase
  firebaseConfig.host = FIREBASE_HOST;
  firebaseConfig.signer.tokens.legacy_token = FIREBASE_AUTH;

  Firebase.begin(&firebaseConfig, &firebaseAuth);
  Firebase.reconnectWiFi(true);

  Serial.println("Connected to Firebase");
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Example threshold values, adjust as necessary
  int gasValue = analogRead(A0);
  bool buzzerState = (t > 38 || h > 90 || gasValue > 900);

  // Control the buzzer
  if (buzzerState) {
    digitalWrite(BUZZER_PIN, HIGH); // Turn buzzer ON
  } else {
    digitalWrite(BUZZER_PIN, LOW);  // Turn buzzer OFF
  }

  // Print sensor values to Serial Monitor
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" °C, Humidity: ");
  Serial.print(h);
  Serial.print(" %, Gas: ");
  Serial.print(gasValue);
  Serial.print(", Buzzer: ");
  Serial.println(buzzerState ? "ON" : "OFF");

  // Send data to Firebase with retry logic
  bool success = false;
  int retryCount = 3;
  String path = "/blocks/block1/node1";

  for (int i = 0; i < retryCount; i++) {
    if (Firebase.setFloat(firebaseData, path + "/temp", t) &&
        Firebase.setFloat(firebaseData, path + "/humi", h) &&
        Firebase.setInt(firebaseData, path + "/gas", gasValue) &&
        Firebase.setBool(firebaseData, path + "/buzzer", buzzerState)) {
      success = true;
      break;
    } else {
      Serial.println("Failed to send data, retrying...");
      Serial.println("Error: " + firebaseData.errorReason());
      delay(2000); // Wait 2 seconds before retrying
    }
  }

  if (success) {
    Serial.println("Data sent successfully");
  } else {
    Serial.println("Failed to send data after retries");
  }

  delay(5000); // 5 seconds delay between requests
}
