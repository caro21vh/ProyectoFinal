/*
 * =========================================
 *  MACETA INTELIGENTE - ESP32
 * =========================================
 * Componentes:
 *  - Higrómetro FC-28  → Pin GPIO34
 *  - BH1750FVI         → SDA GPIO21 / SCL GPIO22
 *  - LED NeoPixel      → Pin GPIO4
 *
 * Cambio: Se agregan perfiles de planta con
 *         umbrales individuales de humedad y luz
 * =========================================
 */

#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <BH1750.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

const char* SSID     = "TU_RED";
const char* PASSWORD = "TU_PASSWORD";

#define SOIL_PIN       34
#define NEOPIXEL_PIN   4
#define NEOPIXEL_COUNT 10

Adafruit_NeoPixel strip(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
BH1750 lightMeter;
WebServer server(80);

struct PlantProfile {
  String name;
  int    soilDryThreshold;
  float  lightMinLux;
};

PlantProfile profiles[] = {
  { "Suculenta", 2800, 300.0 },
  { "Helecho",   1800, 200.0 },
  { "Cactus",    3200, 500.0 },
  { "Tomate",    1600, 600.0 },
  { "Albahaca",  1800, 400.0 },
  { "Generica",  2000, 300.0 },
};

int   soilRaw       = 0;
int   soilPercent   = 0;
float lightLux      = 0.0;
bool  alertSoil     = false;
bool  alertLight    = false;
String plantType    = "Generica";
int   currentProfile = 5;

void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.setBrightness(80);
  strip.show();

  Wire.begin(21, 22);
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);

  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi conectado! IP: " + WiFi.localIP().toString());

  server.on("/",      HTTP_GET,  handleRoot);
  server.on("/data",  HTTP_GET,  handleData);
  server.on("/plant", HTTP_POST, handleSetPlant);
  server.begin();
}

void loop() {
  server.handleClient();
  readSensors();
  checkAlerts();
  updateNeoPixel();
  delay(200);
}

void readSensors() {
  if (lightMeter.measurementReady()) lightLux = lightMeter.readLightLevel();
  soilRaw     = analogRead(SOIL_PIN);
  soilPercent = map(soilRaw, 4095, 0, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);
}

void checkAlerts() {
  PlantProfile& p = profiles[currentProfile];
  alertSoil  = (soilRaw > p.soilDryThreshold);
  alertLight = (lightLux < p.lightMinLux);
}

void updateNeoPixel() {
  if (alertLight) {
    for (int i = 0; i < NEOPIXEL_COUNT; i++)
      strip.setPixelColor(i, strip.Color(255, 30, 120));
  } else if (alertSoil) {
    for (int i = 0; i < NEOPIXEL_COUNT; i++)
      strip.setPixelColor(i, strip.Color(220, 0, 0));
  } else {
    int ledsOn = map(soilPercent, 0, 100, 0, NEOPIXEL_COUNT);
    for (int i = 0; i < NEOPIXEL_COUNT; i++)
      strip.setPixelColor(i, i < ledsOn ? strip.Color(0, 200, 50) : strip.Color(0, 0, 0));
  }
  strip.show();
}

int findProfile(String name) {
  for (int i = 0; i < 6; i++) if (profiles[i].name == name) return i;
  return 5;
}

void handleRoot() { server.send(200, "text/plain", "Maceta Inteligente activa"); }

void handleData() {
  StaticJsonDocument<256> doc;
  doc["humedad_pct"]    = soilPercent;
  doc["luz_lux"]        = (int)lightLux;
  doc["alerta_agua"]    = alertSoil;
  doc["alerta_luz"]     = alertLight;
  doc["planta"]         = plantType;
  doc["umbral_humedad"] = profiles[currentProfile].soilDryThreshold;
  doc["umbral_luz"]     = (int)profiles[currentProfile].lightMinLux;
  String json;
  serializeJson(doc, json);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleSetPlant() {
  StaticJsonDocument<128> doc;
  deserializeJson(doc, server.arg("plain"));
  plantType      = doc["tipo"].as<String>();
  currentProfile = findProfile(plantType);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"ok\":true}");
}
