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

#define DHT_PIN      16
#define DHT_TYPE     DHT11
#define PIR_PIN      4       // Digital input for the PIR OUT wire
#define LED_GREEN    19      // On when no motion
#define LED_RED      18      // On when motion detected
#define BUZZER_PIN   17      // Active buzzer, sounds while motion is detected

const unsigned long SENSOR_INTERVAL_MS = 2000;   // DHT11 needs at least 2 s between reads
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

bool          lastRawPIR = false;
unsigned long pirChanges = 0;     // how many times the PIR pin has changed state

// =====================================================
// 4. Web page and data endpoint
// =====================================================

// The page itself. It is sent once; after that, JavaScript updates it.
const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Habitat Status</title>
</head>
<body>
  <h1>Habitat Status</h1>
  <p>Temperature: <span id="temp">--</span> &deg;C</p>
  <p>Humidity: <span id="hum">--</span> %</p>
  <p>Motion: <span id="motion">--</span></p>
  <p>PIR pin raw: <span id="raw">--</span></p>
  <p>PIR state changes: <span id="changes">--</span></p>

<script>
  function update() {
    fetch('/data')
      .then(r => r.json())
      .then(d => {
        document.getElementById('temp').textContent    = d.temp === null ? 'waiting' : d.temp.toFixed(0);
        document.getElementById('hum').textContent     = d.hum  === null ? 'waiting' : d.hum.toFixed(0);
        document.getElementById('motion').textContent  = d.motion ? 'Detected' : 'None';
        document.getElementById('raw').textContent     = d.raw;
        document.getElementById('changes').textContent = d.changes;
      })
      .catch(() => {});   // ignore a missed request and try again
  }
  update();
  setInterval(update, 500);   // ask for fresh data twice a second
</script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send_P(200, "text/html", PAGE);
}

void handleData() {
  String json = "{";
  json += "\"temp\":"    + (isnan(temp) ? String("null") : String(temp, 1)) + ",";
  json += "\"hum\":"     + (isnan(hum)  ? String("null") : String(hum, 1)) + ",";
  json += "\"motion\":"  + String(motionState ? "true" : "false") + ",";
  json += "\"raw\":"     + String(digitalRead(PIR_PIN)) + ",";
  json += "\"changes\":" + String(pirChanges);
  json += "}";

  server.send(200, "application/json", json);
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
// 6. Sensor, LED and buzzer control
// =====================================================
void readPIR() {
  bool raw = (digitalRead(PIR_PIN) == HIGH);

  if (raw != lastRawPIR) {
    lastRawPIR = raw;
    pirChanges++;
    Serial.println(raw ? "PIR changed: HIGH" : "PIR changed: LOW");
  }

  motionState = raw;

  // Green = no motion, red = motion
  digitalWrite(LED_GREEN, motionState ? LOW : HIGH);
  digitalWrite(LED_RED,   motionState ? HIGH : LOW);

  // Buzzer sounds while motion is detected
  digitalWrite(BUZZER_PIN, motionState ? HIGH : LOW);
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
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  dht.begin();

  Serial.println("PIR warming up, please wait...");
  delay(PIR_WARMUP_MS);
  Serial.println("PIR ready.");

  connectWiFi();

  server.on("/", handleRoot);
  server.on("/data", handleData);
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
