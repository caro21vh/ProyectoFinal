/*
 * =========================================
 *  MACETA INTELIGENTE - ESP32
 * =========================================
 * Equipo: Marisa Méndez | Carolina Montero | 
 *         Diana Obando  | Carolina Vargas  | Albert Vega
 * =========================================
 */

#include <WiFi.h>
#include <WebServer.h>

// ---- Credenciales WiFi ----
const char* SSID     = "TU_RED";
const char* PASSWORD = "TU_PASSWORD";

WebServer server(80);

void setup() {
  Serial.begin(115200);

  Serial.print("Conectando a WiFi");
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handleRoot);
  server.begin();
  Serial.println("Servidor HTTP iniciado.");
}

void loop() {
  server.handleClient();
}

void handleRoot() {
  server.send(200, "text/plain", "Maceta Inteligente activa");
}
