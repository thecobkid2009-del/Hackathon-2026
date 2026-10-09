#include <WiFi.h>
#include <WebServer.h>

// CHANGE THESE
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

WebServer server(80);

// Simulated habitat readings
float temperature = 22.5;
float humidity = 45.0;
float oxygen = 98.0;
float water = 85.0;

void handleData() {
  // Generate small changes in simulated readings
  temperature = 22.5 + random(-20, 21) / 10.0;
  humidity = 45.0 + random(-50, 51) / 10.0;

  String json = "{";
  json += "\"temperature\":" + String(temperature, 1) + ",";
  json += "\"humidity\":" + String(humidity, 1) + ",";
  json += "\"oxygen\":" + String(oxygen, 1) + ",";
  json += "\"water\":" + String(water, 1);
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Mars Habitat Mission Control</title>
  <style>
    body {
      background: #101522;
      color: white;
      font-family: Arial, sans-serif;
      text-align: center;
      padding: 20px;
    }
    h1 { color: #ff7043; }
    .grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
      gap: 20px;
      max-width: 900px;
      margin: auto;
    }
    .card {
      background: #1c2538;
      padding: 25px;
      border-radius: 16px;
    }
    .value {
      font-size: 32px;
      font-weight: bold;
      color: #4fc3f7;
    }
  </style>
</head>
<body>

<h1>MARS HABITAT</h1>
<p>Mission Control — Live Environmental Monitoring</p>
<p><strong>DEMO MODE:</strong> Simulated sensor readings</p>

<div class="grid">
  <div class="card">
    <h3>Temperature</h3>
    <div class="value" id="temp">--</div>
  </div>

  <div class="card">
    <h3>Humidity</h3>
    <div class="value" id="humidity">--</div>
  </div>

  <div class="card">
    <h3>Oxygen Reserves</h3>
    <div class="value" id="oxygen">--</div>
  </div>

  <div class="card">
    <h3>Water Reserves</h3>
    <div class="value" id="water">--</div>
  </div>
</div>

<script>
async function updateData() {
  try {
    const response = await fetch('/data');
    if (!response.ok) throw new Error('Request failed');

    const data = await response.json();

    document.getElementById('temp').textContent =
      data.temperature + ' °C';

    document.getElementById('humidity').textContent =
      data.humidity + ' %';

    document.getElementById('oxygen').textContent =
      data.oxygen + ' %';

    document.getElementById('water').textContent =
      data.water + ' %';

  } catch (error) {
    console.error('Connection error:', error);
  }
}

updateData();
setInterval(updateData, 1000);
</script>

</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);

  Serial.print("Connecting to Wi-Fi");

  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startTime < 20000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWi-Fi connection failed.");
    return;
  }

  Serial.println("\nConnected!");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);

  server.begin();

  Serial.println("Mars Habitat server started!");
}

void loop() {
  server.handleClient();
}