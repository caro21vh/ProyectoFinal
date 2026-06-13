/*
 * =========================================
 *  MACETA INTELIGENTE - ESP32
 * =========================================
 * Componentes:
 *  - Higrometro FC-28    -> Pin GPIO34
 *  - Sensor nivel agua   -> Pin GPIO32
 *  - BH1750FVI           -> SDA GPIO21 / SCL GPIO22
 *  - LED NeoPixel        -> Pin GPIO4
 *  - Buzzer              -> Pin GPIO14
 *
 * Este archivo solo orquesta los componentes (clases):
 *  - SensorReader      : lectura de sensores
 *  - PlantProfile      : umbrales por tipo de planta
 *  - AlertManager      : calculo de estados de alerta
 *  - BuzzerController  : sonidos
 *  - NeoPixelController: luces / estados visuales
 *  - WebApiServer      : API HTTP (WiFi)
 * =========================================
 */

#include <WiFi.h>
#include "SensorReader.h"
#include "PlantProfile.h"
#include "AlertManager.h"
#include "BuzzerController.h"
#include "NeoPixelController.h"
#include "WebApiServer.h"

const char* SSID     = "Maceta";
const char* PASSWORD = "holaash1";

#define SOIL_PIN              34
#define WATER_PIN             32
#define BUZZER_PIN            14
#define NEOPIXEL_PIN          4
#define NEOPIXEL_COUNT        10
#define WATER_EMPTY_THRESHOLD 300

// ---- Perfiles de planta ----
// soilDryThreshold: valor RAW del higrometro -> MAS ALTO = tolera mas sequedad
// lightMinLux: luz minima requerida -> MAS ALTO = necesita mas luz directa
PlantProfile profiles[] = {
  PlantProfile("Suculenta", 3000, 250.0),  // tolera mucha sequedad, luz media
  PlantProfile("Cactus",    3300, 600.0),  // tolera mucha sequedad, mucha luz directa
  PlantProfile("Tomate",    2000, 700.0),  // riego moderado, necesita mucha luz
  };
const int PROFILE_COUNT = 3;

int    currentProfile = 1;
String plantType       = "Suculenta";

// ---- Componentes ----
SensorReader       sensors(SOIL_PIN, WATER_PIN);
AlertManager       alerts;
BuzzerController   buzzer(BUZZER_PIN);
NeoPixelController leds(NEOPIXEL_PIN, NEOPIXEL_COUNT);

int findProfile(const String& name) {
  for (int i = 0; i < PROFILE_COUNT; i++) {
    if (profiles[i].getName() == name) return i;
  }
  return 5;
}

WebApiServer api(80, sensors, alerts, buzzer,
                 profiles, PROFILE_COUNT,
                 currentProfile, plantType,
                 findProfile);

unsigned long lastRead = 0;

void setup() {
  Serial.begin(115200);

  buzzer.begin();
  leds.begin();
  sensors.begin();

  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi conectado! IP: " + WiFi.localIP().toString());

  api.begin();
  buzzer.beepWifiConnected();
}

void loop() {
  api.handleClient();

  if (millis() - lastRead >= 200) {
    lastRead = millis();

    sensors.read();
    sensors.printDebug();

    const PlantProfile& profile = profiles[currentProfile];
    bool lightJustRecovered = alerts.update(
      sensors.getSoilRaw(), sensors.getLightLux(), sensors.getWaterRaw(),
      profile, WATER_EMPTY_THRESHOLD
    );

    buzzer.update(alerts.isWaterEmpty(), alerts.isSoilDry(), lightJustRecovered);

    leds.update(alerts.isGrowLightOn(), alerts.isSoilDry(),
                 alerts.needsWaterAttention(), sensors.getSoilPercent());
  }
}
