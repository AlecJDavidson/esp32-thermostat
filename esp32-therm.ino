#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>

// Constants
#define DHTPIN 4       // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11  // DHT 11 (AM2302)
const char *ssid = "<your-ssid>";
const char *password = "<your-wifi-password>";
#define RELAY_HEAT_PIN 12  // GPIO pin for heat relay
#define RELAY_COOL_PIN 14  // GPIO pin for cool relay

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

DHT dht(DHTPIN, DHTTYPE);
float temperature = 0.0;
float setTemperature = 68.0;  // Temperature threshold in Fahrenheit

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Connect to WiFi network with SSID and password
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Serve HTML page with buttons and input box
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<!DOCTYPE html>";
    html += "<html><head><title>ESP32 Thermostat</title></head>";
    html += "<body>";
    html += "<h1>Temperature: " + String(temperature) + " °F</h1>";
    html += "<button onclick='location.href=\"/mode/heat\"'>Activate Heat</button><br>";
    html += "<button onclick='location.href=\"/mode/cool\"'>Activate Cool</button><br>";
    html += "<button onclick='location.href=\"/mode/off\"'>Turn Off</button><br>";
    // Input box for setting temperature
    html += "<form method='post' action='/settemp'>";
    html += "<label>Set Temperature (°C): </label>";
    html += "<input type='number' name='temp' min='-50' max='100' value='temp'>";
    html += "<button type='submit'>Submit</button><br>";
    html += "</form>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  // Setup WebServer routes
  server.on("/mode/heat", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(RELAY_HEAT_PIN, HIGH);  // Turn on heat
    digitalWrite(RELAY_COOL_PIN, LOW);   // Turn off cool
    Serial.println("Heat mode activated");
    request->send(200, "text/plain", "Heat mode activated");
  });

  server.on("/mode/cool", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(RELAY_HEAT_PIN, LOW);   // Turn off heat
    digitalWrite(RELAY_COOL_PIN, HIGH);  // Turn on cool
    Serial.println("Cool mode activated");
    request->send(200, "text/plain", "Cool mode activated");
  });

  server.on("/mode/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(RELAY_HEAT_PIN, LOW);  // Turn off heat
    digitalWrite(RELAY_COOL_PIN, LOW);  // Turn off cool
    Serial.println("No appliance should be running");
    request->send(200, "text/plain", "No appliance should be running");
  });

  server.on("/temp/c", HTTP_GET, [](AsyncWebServerRequest *request) {
    String tempString = String(temperature);
    request->send(200, "text/plain", tempString);
  });

  server.on("/settemp", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Get the "temp" parameter from the request
    AsyncWebParameter *param = request->getParam("temp");

    if (param && param->isPost()) {              // Check if the parameter exists and is a POST parameter
      float newTemp = param->value().toFloat();  // Convert the parameter value to float

      if (!isnan(newTemp)) {
        setTemperature = newTemp;
      }

      Serial.println("Set temperature to " + String(setTemperature) + "F");
      request->send(200, "text/plain", "Set temperature to " + String(setTemperature) + "F");
    } else {
      request->send(400, "text/plain", "Invalid or missing 'temp' parameter");
    }
  });

  // Start server
  server.begin();
}

void loop() {
  static unsigned long previousMillis = 0;
  const long interval = 2000;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    float newTemp = dht.readTemperature();
    if (!isnan(newTemp)) {
      // Set to Fahrenheit
      temperature = round((newTemp * 9 / 5) + 32);
    }
    // Turn off heating system if temperature is greater than or equal to set threshold
    if (temperature >= setTemperature && digitalRead(RELAY_HEAT_PIN) == HIGH) {
      digitalWrite(RELAY_HEAT_PIN, LOW);  // Turn off heat
      Serial.println("Heat mode deactivated");
    }
    // Turn off cooling system if temperature is less than or equal to set threshold
    if (temperature <= setTemperature && digitalRead(RELAY_COOL_PIN) == HIGH) {
      digitalWrite(RELAY_COOL_PIN, LOW);  // Turn off cool
      Serial.println("Cool mode deactivated");
    }
  }
}
