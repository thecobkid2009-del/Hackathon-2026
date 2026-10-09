
#include <DHT.h>
#include <WiFi.h>
#include <WebServer.h>

// Wi-Fi credentials
const char* ssid = "ESP NET";
const char* password = "TestESP32";

#define LDR_PIN 32
#define DHT_PIN 16
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);
WebServer server(80);

float temp = NAN;
float hum = NAN;
int lightState = HIGH;

void handleRoot() {
  String html = "<html><head>";
  html += "<meta http-equiv='refresh' content='2'>";
  html += "</head><body>";

  html += "<h1>Habitat Status</h1>";

  if (isnan(temp) || isnan(hum)) {
    html += "<p>Waiting for sensor readings...</p>";
  } else {
    html += "Temperature: " + String(temp, 1) + " &deg;C<br>";
    html += "Humidity: " + String(hum, 1) + " %<br>";
  }

  html += "Light status: ";
  html += (lightState == HIGH) ? "HIGH" : "LOW";
  html += "<br>";
  html += "<p>Lux measurement unavailable with digital-only LDR.</p>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(9600);

  dht.begin();
  pinMode(LDR_PIN, INPUT);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected. IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  server.handleClient();

  static unsigned long lastRead = 0;

  if (millis() - lastRead >= 2000) {
    lastRead = millis();

    temp = dht.readTemperature();
    hum = dht.readHumidity();
    lightState = digitalRead(LDR_PIN);

    Serial.print("Temperature: ");
    Serial.print(temp);
    Serial.print(" C | Humidity: ");
    Serial.print(hum);
    Serial.print(" % | Light state: ");
    Serial.println(lightState == HIGH ? "HIGH" : "LOW");
  }
}
