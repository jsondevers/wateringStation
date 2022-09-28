#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <string.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_sensor.h>
#include <DHT_U.h>

#include "main.h"

struct sensorReadings {
  uint moisture[NUM_MOISTURE_SENSORS];
  uint avgmoisture;
  float humidity;
  float temperature;
};

TwoWire tw = TwoWire(1); 
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &tw, OLED_RESET); 
WebServer server(API_PORT); 
DHT_Unified dht(PIN_DHT, DHT_TYPE);


struct sensorReadings sensorReadings = {0};


void moistureReadings() {
  digitalWrite(PIN_MOISTURE_VCC, HIGH); 
  delay(100); 

  
  for (int i = 0; i < NUM_MOISTURE_SENSORS; i++) {
    int adc = analogRead(PINS_MOISTURE[i]);
 
    if (adc >= MAX_MOISTURE_VALUE && adc <= MIN_MOISTURE_VALUE) {
    
      sensorReadings.moisture[i] = uint(map(adc, MIN_MOISTURE_VALUE, MAX_MOISTURE_VALUE, 0, 100));
    }
  }

  digitalWrite(PIN_MOISTURE_VCC, LOW);


  int avgmoisture = 0;
  for (int i = 0; i < NUM_MOISTURE_SENSORS; i++) {
    avgmoisture += sensorReadings.moisture[i];
  }
  sensorReadings.avgmoisture = (uint) round(avgmoisture / NUM_MOISTURE_SENSORS);
}

void tempHumidity() {
  sensors_event_t event;

  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    sensorReadings.temperature = 0;
  } else {
    sensorReadings.temperature = event.temperature;
  }

  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    sensorReadings.humidity = 0;
  } else {
    sensorReadings.humidity = event.relative_humidity;
  }
}

void sensorReadings() {
  moistureReadings();
  tempHumidity();
}

void pumpWater(int seconds) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.printf("Watering for %is...", seconds);
  display.setCursor(0, 10);
  display.printf("Then waiting %is...", WAIT_SECONDS);
  display.display();
  digitalWrite(PIN_PUMP, HIGH); 
  delay(seconds * 1000); 
  digitalWrite(PIN_PUMP, LOW);
}

void displayReadings() {
  int spacing = int(OLED_HEIGHT / 3);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.printf("%s %i%%", MOISTURE_MSG, sensorReadings.avgmoisture);
  display.setCursor(0, spacing);
  display.printf("%s %.1f%cC", AIR_TEMP_MSG, sensorReadings.temperature, (char)247);
  display.setCursor(0, spacing * 2);
  display.printf("%s %.1f%%", HUMIDITY_MSG, sensorReadings.humidity);
  display.setCursor(0, spacing * 3);  
  display.display();
}

void getReadingsJSON(char *dest, size_t size) {
  StaticJsonDocument<JSON_ARRAY_SIZE(NUM_MOISTURE_SENSORS) + JSON_OBJECT_SIZE(4) + 50> json;

  json.createNestedArray("moisture_values");
  JsonArray moistureValues = json["moisture_values"].as<JsonArray>();

  for (int i = 0; i < NUM_MOISTURE_SENSORS; i++) {
    moistureValues.add(sensorReadings.moisture[i]);
  }

  json["avg_moisture"].set(sensorReadings.avgmoisture);
  json["humidity"].set(sensorReadings.humidity);
  json["temperature"].set(sensorReadings.temperature);

  serializeJson(json, dest, size);
}

void handleNotFound() {
  server.send(404, "text/plain", "Page Not Found");
}

void handleReadings() {
  char buffer[JSON_BUFF_SIZE];
  getReadingsJSON(buffer, JSON_BUFF_SIZE);
  server.send(200, "text/json", String(buffer));
}

void listenerLoop(void * pvParameters) {
  while(true) { 
    server.handleClient();
  }
}

void loop() {
  sensorReadings();
  displayReadings();

  if (sensorReadings.avgmoisture != 0 && sensorReadings.avgmoisture <= LOW_MOISTURE_TRIGGER) {
    while (sensorReadings.avgmoisture < TARGET_MOISTURE) {
      pumpWater(PUMP_SECONDS);
      displayReadings();
      delay(WAIT_SECONDS * 1000);
      sensorReadings(); // Read the sensors again
      displayReadings();
    }
  } else {
    delay(WAIT_SECONDS * 1000);
  }
}

void setupWireless() {

  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_STA);

  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  bool inverter = true; /
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Connecting to:");
    display.setCursor(0, 10);
    display.print(WIFI_SSID);
    display.setCursor(0, 20);

    if (inverter == true) {
      display.print("\\");
    } else {
      display.print("/");
    }

    display.display();
    inverter  = !inverter;
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Connected!");
  display.setCursor(0, 10);
  display.print("IP address:");
  display.setCursor(0, 20);
  char buffer[15]; 
  WiFi.localIP().toString().toCharArray(buffer, 15);
  Serial.println(buffer);
  display.print(buffer);
  display.display();
  delay(5000); 

  server.onNotFound(handleNotFound);
  server.on(API_ENDPOINT, HTTP_GET, handleReadings);
  server.begin();


  xTaskCreate(listenerLoop, "HTTP Server Listener", 4096, NULL, 1, NULL);
}

void setup() {
  dht.begin();
  tw.begin(PIN_SDA, PIN_SCL);
  Serial.begin(9600);


  pinMode(PIN_PUMP, OUTPUT);
  digitalWrite(PIN_PUMP, LOW);

  pinMode(PIN_MOISTURE_VCC, OUTPUT);
  digitalWrite(PIN_MOISTURE_VCC, LOW);

  delay(3000); 
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1.5);

  setupWireless();
}