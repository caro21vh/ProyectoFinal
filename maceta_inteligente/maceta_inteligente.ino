/*
 * =========================================
 *  MACETA INTELIGENTE - ESP32
 * =========================================
 * Componentes:
 *  - Higrómetro FC-28    → Pin GPIO34
 *  - Sensor nivel agua   → Pin GPIO32
 *  - BH1750FVI           → SDA GPIO21 / SCL GPIO22
 *  - LED NeoPixel        → Pin GPIO4
 *  - Buzzer              → Pin GPIO14
 *
 * Cambios: - Sonido cuando la tierra está seca.
 *          - cada combinacion de alertSoil / alertLight / alertWater 
 *            produce un comportamiento de luces y sonido distinto y observable.
 *          - Cuando el grow light está compensando luz (alertLight = true)
 *            y además la tierra está seca (alertSoil = true), cada 3 segundos
 *            el grow light hace un "parpadeo" rápido a otro color para avisar
 *            que también hay que regar / revisar el tanque.
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

#define SOIL_PIN              34
#define WATER_PIN             32
#define BUZZER_PIN            14
#define NEOPIXEL_PIN          4
#define NEOPIXEL_COUNT        10
#define WATER_EMPTY_THRESHOLD 300

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

int    soilRaw        = 0;
int    soilPercent    = 0;
int    waterRaw       = 0;
float  lightLux       = 0.0;
bool   alertSoil      = false;
bool   alertLight     = false;
bool   alertWater     = false;
bool   growLightOn    = false;
String plantType      = "Generica";
int    currentProfile = 5;

unsigned long lastRead    = 0;
unsigned long lastTankBeep = 0;
unsigned long lastSoilBeep    = 0;   // control de beep por tierra seca
unsigned long lastWaterBlink  = 0;   // control del parpadeo "ocupa regar" cada 3s

const unsigned long BLINK_DURATION   = 250;
const unsigned long BLINK_INTERVAL   = 3000; // cada 3 segundos

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

  server.on("/",       HTTP_GET,  handleRoot);
  server.on("/data",   HTTP_GET,  handleData);
  server.on("/plant",  HTTP_POST, handleSetPlant);
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
  waterRaw    = analogRead(WATER_PIN);
  Serial.printf("Humedad: %d%% (raw:%d) | Luz: %.1f lux | Agua tanque raw: %d\n",
                soilPercent, soilRaw, lightLux, waterRaw);
}

void checkAlerts() {
  PlantProfile& p = profiles[currentProfile];
  bool newAlertSoil  = (soilRaw > p.soilDryThreshold);
  bool newAlertLight = (lightLux < p.lightMinLux);
  bool newAlertWater = (waterRaw < WATER_EMPTY_THRESHOLD);

  if (newAlertWater && millis() - lastTankBeep > 5000) {
    beepTanqueVacio();
    lastTankBeep = millis();
  }

  if (newAlertSoil && millis() - lastSoilBeep > 5000) {
    beepTierraSeca();
    lastSoilBeep = millis();
  }

  if (alertLight && !newAlertLight) beepLuzOk();

  alertSoil  = newAlertSoil;
  alertLight = newAlertLight;
  alertWater = newAlertWater;
  growLightOn = alertLight;
}

void updateNeoPixel() {

  bool needsWaterAttention = (alertSoil || alertWater);

  if (growLightOn) {
    if (needsWaterAttention) {
      // ESTADO 1: grow light con parpadeo de aviso cada 3 segundos
      unsigned long now = millis();
      unsigned long cyclePos = (now - lastWaterBlink) % BLINK_INTERVAL;

      if (cyclePos < BLINK_DURATION) {
        // Parpadeo de aviso: color ambar/amarillo
        for (int i = 0; i < NEOPIXEL_COUNT; i++)
          strip.setPixelColor(i, strip.Color(255, 180, 0));
      } else {
        // Color normal del grow light
        for (int i = 0; i < NEOPIXEL_COUNT; i++)
          strip.setPixelColor(i, strip.Color(255, 30, 120));
      }

      // Reiniciar referencia del ciclo si ha pasado un intervalo completo
      if (now - lastWaterBlink >= BLINK_INTERVAL) {
        lastWaterBlink = now;
      }

    } else {
      // ESTADO 2: grow light fijo, sin parpadeo
      for (int i = 0; i < NEOPIXEL_COUNT; i++)
        strip.setPixelColor(i, strip.Color(255, 30, 120));
    }

  } else if (alertSoil) {
    // ESTADO 3: tierra seca, sin problema de luz -> parpadeo rojo
    static bool toggle = false;
    static unsigned long lastToggle = 0;
    if (millis() - lastToggle > 500) { toggle = !toggle; lastToggle = millis(); }
    for (int i = 0; i < NEOPIXEL_COUNT; i++)
      strip.setPixelColor(i, toggle ? strip.Color(220, 0, 0) : strip.Color(0, 0, 0));

  } else {
    // ESTADO 4: todo normal -> barra verde segun humedad
    int ledsOn = map(soilPercent, 0, 100, 0, NEOPIXEL_COUNT);
    for (int i = 0; i < NEOPIXEL_COUNT; i++)
      strip.setPixelColor(i, i < ledsOn ? strip.Color(0, 200, 50) : strip.Color(0, 0, 0));
  }

  strip.show();
}

void beepWifiConnected() {
  ledcAttachPin(BUZZER_PIN, 0);
  ledcWriteTone(0, 800);  delay(80); ledcWriteTone(0, 0); delay(40);
  ledcWriteTone(0, 1200); delay(80); ledcWriteTone(0, 0); delay(40);
  ledcWriteTone(0, 1800); delay(120); ledcWriteTone(0, 0);
}

void beepTanqueVacio() {
  ledcAttachPin(BUZZER_PIN, 0);
  ledcWriteTone(0, 400); delay(200); ledcWriteTone(0, 0); delay(100);
  ledcWriteTone(0, 400); delay(200); ledcWriteTone(0, 0);
}

void beepTierraSeca() {
  ledcAttachPin(BUZZER_PIN, 0);
  ledcWriteTone(0, 900); delay(120); ledcWriteTone(0, 0); delay(80);
  ledcWriteTone(0, 900); delay(120); ledcWriteTone(0, 0);
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
  doc["alerta_tanque"]  = alertWater;
  doc["grow_light"]     = growLightOn;
  doc["planta"]         = plantType;
  doc["umbral_humedad"] = profiles[currentProfile].soilDryThreshold;
  doc["umbral_luz"]     = (int)profiles[currentProfile].lightMinLux;
  doc["agua_tanque_raw"]= waterRaw;
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
  Serial.printf("Planta: %s | Umbral humedad: %d | Umbral luz: %.0f lux\n",
                plantType.c_str(),
                profiles[currentProfile].soilDryThreshold,
                profiles[currentProfile].lightMinLux);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleBuzzer() {
  beepNotification();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"ok\":true}");
}
