#include <WiFi.h>
#include <WebServer.h>

// Wifi SSID and password
const char* ssid = "Jacobs S25 Ultra";
const char* password = "P0tat0Chip";



// Simulated habitat examples
float temperature = 22.5;
float humidity = 45.0;
float oxygen = 98.0;
float water = 85.0;

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);

  Serial.print("Connecting to Wi-Fi");
}


