/*
 * Habitat Status: ESP32 web monitor
 * DHT11 temperature/humidity + PIR motion, served on a local web page.
 * Requires the Adafruit "DHT sensor library" and "Adafruit Unified Sensor".
 */

// =====================================================
// 1. Libraries
// =====================================================
#include <DHT.h>
#include <WiFi.h>
#include <WebServer.h>

// =====================================================
// 2. Configuration
// =====================================================
const char* ssid     = "ESP NET";
const char* password = "TestESP32";

#define DHT_PIN   16
#define DHT_TYPE  DHT11
#define PIR_PIN   4        // GPIO34 is input-only, which is fine for a PIR

const unsigned long SENSOR_INTERVAL_MS = 2000;
const unsigned long WIFI_TIMEOUT_MS    = 20000;
const unsigned long PIR_WARMUP_MS      = 30000;

// =====================================================
// 3. Objects and global state
// =====================================================
DHT dht(DHT_PIN, DHT_TYPE);
WebServer server(80);

float temp = NAN;
float hum  = NAN;
bool  motionState = false;

// =====================================================
// 4. Web page
// =====================================================
void handleRoot() {
  String html = "<html><head><meta http-equiv='refresh' content='2'></head><body>";
  html += "<h1>Habitat Status</h1>";

  if (isnan(temp) || isnan(hum)) {
    html += "<p>Waiting for sensor readings...</p>";
  } else {
    html += "Temperature: " + String(temp, 0) + " &deg;C<br>";   // DHT11 = whole degrees
    html += "Humidity: " + String(hum, 0) + " %<br>";
  }

  html += "Motion: " + String(motionState ? "Detected" : "None") + "<br>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

// =====================================================
// 5. Wi-Fi
// =====================================================
void connectWiFi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > WIFI_TIMEOUT_MS) {
      Serial.println();
      Serial.println("Wi-Fi timeout, retrying...");
      WiFi.disconnect();
      WiFi.begin(ssid, password);
      start = millis();
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected. IP address: ");
  Serial.println(WiFi.localIP());
}

// =====================================================
// 6. Sensor reading
// =====================================================
void readPIR() {
  motionState = (digitalRead(PIR_PIN) == HIGH);   // every pass, so short pulses aren't missed
}

void readDHT() {
  static unsigned long lastRead = 0;
  if (millis() - lastRead < SENSOR_INTERVAL_MS) return;
  lastRead = millis();

  temp = dht.readTemperature();
  hum  = dht.readHumidity();

  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print(" C | Humidity: ");
  Serial.print(hum);
  Serial.print(" % | Motion: ");
  Serial.println(motionState ? "Yes" : "No");
}

// =====================================================
// 7. Setup
// =====================================================
void setup() {
  Serial.begin(9600);

  pinMode(PIR_PIN, INPUT);
  dht.begin();

  // Let the PIR settle before trusting its output
  Serial.println("PIR warming up, please wait...");
  delay(PIR_WARMUP_MS);
  Serial.println("PIR ready.");

  connectWiFi();

  server.on("/", handleRoot);
  server.begin();
}

// =====================================================
// 8. Main loop
// =====================================================
void loop() {
  server.handleClient();
  readPIR();
  readDHT();
}
