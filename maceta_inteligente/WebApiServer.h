#ifndef WEB_API_SERVER_H
#define WEB_API_SERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "SensorReader.h"
#include "AlertManager.h"
#include "PlantProfile.h"
#include "BuzzerController.h"

/*
 * WebApiServer
 * -------------
 * Encapsula el WebServer y expone las rutas:
 *  - GET  /        : status simple
 *  - GET  /data    : JSON con lecturas y estados de alerta
 *  - POST /plant   : cambia el perfil de planta activo
 *  - POST /buzzer  : dispara un beep de notificacion manual
 *
 * Recibe referencias a los demas controladores para poder
 * leer su estado y construir las respuestas.
 */
class WebApiServer {
  public:
    // Callback que el main usa para resolver un nombre de planta
    // a un indice de perfil (findProfile).
    using ProfileResolver = int (*)(const String& name);

    WebApiServer(uint16_t port,
                  SensorReader& sensors,
                  AlertManager& alerts,
                  BuzzerController& buzzer,
                  PlantProfile* profiles, int profileCount,
                  int& currentProfileRef, String& plantTypeRef,
                  ProfileResolver resolver)
      : _server(port), _sensors(sensors), _alerts(alerts), _buzzer(buzzer),
        _profiles(profiles), _profileCount(profileCount),
        _currentProfile(currentProfileRef), _plantType(plantTypeRef),
        _resolver(resolver) {}

    void begin() {
      _server.on("/",       HTTP_GET,  [this]() { handleRoot(); });
      _server.on("/data",   HTTP_GET,  [this]() { handleData(); });
      _server.on("/plant",  HTTP_POST, [this]() { handleSetPlant(); });
      _server.on("/buzzer", HTTP_POST, [this]() { handleBuzzer(); });
      _server.begin();
    }

    void handleClient() {
      _server.handleClient();
    }

  private:
    WebServer _server;
    SensorReader&     _sensors;
    AlertManager&     _alerts;
    BuzzerController& _buzzer;
    PlantProfile*     _profiles;
    int               _profileCount;
    int&              _currentProfile;
    String&           _plantType;
    ProfileResolver   _resolver;

    void handleRoot() {
      _server.send(200, "text/plain", "Maceta Inteligente activa");
    }

    void handleData() {
      StaticJsonDocument<256> doc;
      const PlantProfile& p = _profiles[_currentProfile];

      doc["humedad_pct"]     = _sensors.getSoilPercent();
      doc["luz_lux"]         = (int)_sensors.getLightLux();
      doc["alerta_agua"]     = _alerts.isSoilDry();
      doc["alerta_luz"]      = _alerts.isLightLow();
      doc["alerta_tanque"]   = _alerts.isWaterEmpty();
      doc["grow_light"]      = _alerts.isGrowLightOn();
      doc["planta"]          = _plantType;
      doc["umbral_humedad"]  = p.getSoilDryThreshold();
      doc["umbral_luz"]      = (int)p.getLightMinLux();
      doc["agua_tanque_raw"] = _sensors.getWaterRaw();

      String json;
      serializeJson(doc, json);
      _server.sendHeader("Access-Control-Allow-Origin", "*");
      _server.send(200, "application/json", json);
    }

    void handleSetPlant() {
      StaticJsonDocument<128> doc;
      deserializeJson(doc, _server.arg("plain"));

      _plantType      = doc["tipo"].as<String>();
      _currentProfile = _resolver(_plantType);

      const PlantProfile& p = _profiles[_currentProfile];
      Serial.printf("Planta: %s | Umbral humedad: %d | Umbral luz: %.0f lux\n",
                    _plantType.c_str(), p.getSoilDryThreshold(), p.getLightMinLux());

      _server.sendHeader("Access-Control-Allow-Origin", "*");
      _server.send(200, "application/json", "{\"ok\":true}");
    }

    void handleBuzzer() {
      _buzzer.beepNotification();
      _server.sendHeader("Access-Control-Allow-Origin", "*");
      _server.send(200, "application/json", "{\"ok\":true}");
    }
};

#endif
