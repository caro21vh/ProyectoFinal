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
#define BUZZER_PIN     14
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
unsigned long lastRead = 0;

void setup() {
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  ledcSetup(0, 5000, 8);

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
  server.on("/buzzer", HTTP_POST, handleBuzzer);
  server.begin();

  beepWifiConnected();
}

void loop() {
  server.handleClient();
  if (millis() - lastRead >= 200) {
    lastRead = millis();
    readSensors();
    checkAlerts();
    updateNeoPixel();
  }
}

void readSensors() {
  if (lightMeter.measurementReady()) lightLux = lightMeter.readLightLevel();
  soilRaw     = analogRead(SOIL_PIN);
  soilPercent = map(soilRaw, 4095, 0, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);
  Serial.printf("Humedad: %d%% | Luz: %.1f lux\n", soilPercent, lightLux);
}

void checkAlerts() {
  PlantProfile& p = profiles[currentProfile];
  PlantProfile& p = profiles[currentProfile];
  bool newAlertSoil  = (soilRaw > p.soilDryThreshold);
  bool newAlertLight = (lightLux < p.lightMinLux);
  if (alertLight && !newAlertLight) beepLuzOk();
  alertSoil  = newAlertSoil;
  alertLight = newAlertLight;
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

void beepWifiConnected() {
  ledcAttachPin(BUZZER_PIN, 0);
  ledcWriteTone(0, 800);  delay(80);
  ledcWriteTone(0, 0);    delay(40);
  ledcWriteTone(0, 1200); delay(80);
  ledcWriteTone(0, 0);    delay(40);
  ledcWriteTone(0, 1800); delay(120);
  ledcWriteTone(0, 0);
}

void beepLuzOk() {
  ledcAttachPin(BUZZER_PIN, 0);
  ledcWriteTone(0, 1000); delay(60);
  ledcWriteTone(0, 1400); delay(60);
  ledcWriteTone(0, 1800); delay(100);
  ledcWriteTone(0, 0);
}

void beepNotification() {
  ledcAttachPin(BUZZER_PIN, 0);
  ledcWriteTone(0, 1000); delay(80);
  ledcWriteTone(0, 1500); delay(120);
  ledcWriteTone(0, 0);
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

void handleBuzzer() {
  beepNotification();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"ok\":true}");
}

