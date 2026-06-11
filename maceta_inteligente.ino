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
#include <Wire.h>
#include <BH1750.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

const char* SSID     = "TU_RED";
const char* PASSWORD = "TU_PASSWORD";

#define SOIL_PIN  34
#define NEOPIXEL_PIN   4
#define NEOPIXEL_COUNT 10

Adafruit_NeoPixel strip(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
WebServer server(80);

int soilRaw     = 0;
int soilPercent = 0;
float lightLux    = 0.0;

void setup() {
  Serial.begin(115200);

  strip.begin();
  strip.setBrightness(80);
  strip.show();

Wire.begin(21, 22);
  if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("Error: BH1750 no encontrado.");
  } else {
    Serial.println("BH1750 iniciado correctamente.");
  }

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
  readSensors();
  updateNeoPixel();
  delay(200);
}

void readSensors() {
  if (lightMeter.measurementReady()) {
    lightLux = lightMeter.readLightLevel();
  }
  soilRaw      = analogRead(SOIL_PIN);
  soilPercent  = map(soilRaw, 4095, 0, 0, 100);
  soilPercent  = constrain(soilPercent, 0, 100);

  lightRaw      = analogRead(LIGHT_PIN);
  lightDetected = (lightRaw > LIGHT_THRESHOLD);

  Serial.printf("Humedad: %d%% | Luz: %.1f lux\n", soilPercent, lightLux);
}

void updateNeoPixel() {
  if (lightLux < 300.0) {
    for (int i = 0; i < NEOPIXEL_COUNT; i++)
      strip.setPixelColor(i, strip.Color(255, 30, 120));
  } else {
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
  doc["luz_lux"]     = (int)lightLux;
  String json;
  serializeJson(doc, json);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}
