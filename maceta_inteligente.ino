/*
 * =========================================
 *  MACETA INTELIGENTE - ESP32
 * =========================================
 * Equipo: Marisa Méndez | Carolina Montero | 
 *         Diana Obando  | Carolina Vargas  | Albert Vega
 *
 * Componentes:
 *  - Higrómetro FC-28 → Pin GPIO34 (analógico)
 * =========================================
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

const char* SSID     = "TU_RED";
const char* PASSWORD = "TU_PASSWORD";

#define SOIL_PIN  34

WebServer server(80);

int soilRaw     = 0;
int soilPercent = 0;

void setup() {
  Serial.begin(115200);

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
  delay(200);
}

void readSoil() {
  soilRaw     = analogRead(SOIL_PIN);
  soilPercent = map(soilRaw, 4095, 0, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);
  Serial.printf("Humedad: %d%% (raw: %d)\n", soilPercent, soilRaw);
}

void handleRoot() {
  server.send(200, "text/plain", "Maceta Inteligente activa");
}

void handleData() {
  StaticJsonDocument<128> doc;
  doc["humedad_pct"] = soilPercent;
  String json;
  serializeJson(doc, json);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}
