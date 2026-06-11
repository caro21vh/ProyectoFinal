/*
 * =========================================
 *  MACETA INTELIGENTE - ESP32
 * =========================================
 * Equipo: Marisa Méndez | Carolina Montero | 
 *         Diana Obando  | Carolina Vargas  | Albert Vega
 *
 * Componentes:
 *  - Higrómetro FC-28 → Pin GPIO34 (analógico)
 *  - LED NeoPixel     → Pin GPIO4
 * =========================================
 */

#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

const char* SSID     = "TU_RED";
const char* PASSWORD = "TU_PASSWORD";

#define SOIL_PIN  34
#define LIGHT_PIN 35
#define NEOPIXEL_PIN   4
#define NEOPIXEL_COUNT 10
#define LIGHT_THRESHOLD 15

Adafruit_NeoPixel strip(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
WebServer server(80);

int soilRaw     = 0;
int soilPercent = 0;
int  lightRaw     = 0;
bool lightDetected = false;

void setup() {
  Serial.begin(115200);

  strip.begin();
  strip.setBrightness(80);
  strip.show();

  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado! IP: " + WiFi.localIP().toString());

  server.on("/",      HTTP_GET, handleRoot);
  server.on("/data",  HTTP_GET, handleData);
  server.begin();
}

void loop() {
  server.handleClient();
  readSoil();
  updateNeoPixel();
  delay(200);
}

void readSoil() {
  soilRaw     = analogRead(SOIL_PIN);
  soilPercent = map(soilRaw, 4095, 0, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);
  Serial.printf("Humedad: %d%% (raw: %d)\n", soilPercent, soilRaw);
}

void readSensors() {
  soilRaw      = analogRead(SOIL_PIN);
  soilPercent  = map(soilRaw, 4095, 0, 0, 100);
  soilPercent  = constrain(soilPercent, 0, 100);

  lightRaw      = analogRead(LIGHT_PIN);
  lightDetected = (lightRaw > LIGHT_THRESHOLD);

  Serial.printf("Humedad: %d%% | Luz RAW: %d | Detectada: %s\n",
                soilPercent, lightRaw, lightDetected ? "SI" : "NO");
}

void updateNeoPixel() {
  if (!lightDetected) {
    // Sin luz: todos azul
    for (int i = 0; i < NEOPIXEL_COUNT; i++)
      strip.setPixelColor(i, strip.Color(0, 0, 200));
  } else {
    // Con luz: barra verde de humedad
    int ledsOn = map(soilPercent, 0, 100, 0, NEOPIXEL_COUNT);
    for (int i = 0; i < NEOPIXEL_COUNT; i++)
      strip.setPixelColor(i, i < ledsOn ? strip.Color(0, 200, 50) : strip.Color(0, 0, 0));
  }
  strip.show();
}

void handleRoot() {
  server.send(200, "text/plain", "Maceta Inteligente activa");
}

void handleData() {
  StaticJsonDocument<128> doc;
  doc["humedad_pct"] = soilPercent;
  doc["luz_detectada"]  = lightDetected;
  doc["luz_raw"]        = lightRaw;
  String json;
  serializeJson(doc, json);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}
