#include <DHT.h>
#include <WiFi.h>
#include <WebServer.h> 




// Wi-Fi credentials
const char* ssid = "ESP NET";
const char* password = "TestESP32";

float lightLevel;
float temp;
float hum;

#define LDR_PIN 32

#define DHT_PIN 16 //DHT sensor module used for measuring temp and humidity

#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

WebServer server(80);

void handleRoot() {

  String html = "<html><head>";
  html += "<meta http-equiv='refresh' content='2'>";
  html += "</head><body>";

  html += "<h1>Habitat Status</h1>";

  if (isnan(temp) || isnan(hum)) {
    html += "<p>Waiting for sensor readings...</p>";
  } else {
    html += "Temperature: " + String(temp, 1) + " C<br>";
    html += "Humidity: " + String(hum, 1) + " %<br>";
    html += "Light level: " + String(lightLevel, 1) + " Lux<br>";

  }

  html += "</body></html>";

  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(9600);
  dht.begin(); 

  Serial.print("Debug Options");
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");

  // Wait until connected
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
  temp = dht.readTemperature();  // Celsius 
  hum  = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) { 

    Serial.println("Sensor read error! Check wiring."); 

    return; 

  } 

  Serial.print("Temperature: "); 

  Serial.print(temp, 1); 

  Serial.print(" C  Humidity: "); 

  Serial.print(hum, 1); 

  Serial.println(" %"); 



//Light sensor LDR
lightLevel = analogRead(LDR_PIN);
  handleRoot();
  delay(2000);
  
}

 

