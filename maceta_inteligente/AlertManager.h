#ifndef ALERT_MANAGER_H
#define ALERT_MANAGER_H

#include <Arduino.h>
#include "PlantProfile.h"

/*
 * AlertManager
 * -------------
 * Recibe las lecturas de los sensores y el perfil de planta
 * activo, y calcula los 3 estados de alerta del sistema:
 *  - alertSoil  : tierra seca
 *  - alertLight : luz insuficiente (activa el grow light)
 *  - alertWater : tanque de agua vacio
 *
 * Tambien expone si el "grow light" debe estar encendido.
 */
class AlertManager {
  public:
    AlertManager()
      : _alertSoil(false), _alertLight(false), _alertWater(false),
        _growLightOn(false), _wasLightAlert(false) {}

    // Actualiza los estados de alerta segun las lecturas actuales.
    // Retorna true si la luz acaba de pasar de "insuficiente" a "ok"
    // (util para disparar el beep de "luz ok").
    bool update(int soilRaw, float lightLux, int waterRaw,
                const PlantProfile& profile, int waterEmptyThreshold) {

      bool newAlertSoil  = profile.isSoilDry(soilRaw);
      bool newAlertLight = profile.isLightLow(lightLux);
      bool newAlertWater = (waterRaw < waterEmptyThreshold);

      bool lightJustRecovered = (_alertLight && !newAlertLight);

      _alertSoil   = newAlertSoil;
      _alertLight  = newAlertLight;
      _alertWater  = newAlertWater;
      _growLightOn = _alertLight;

      return lightJustRecovered;
    }

    bool isSoilDry()      const { return _alertSoil; }
    bool isLightLow()     const { return _alertLight; }
    bool isWaterEmpty()   const { return _alertWater; }
    bool isGrowLightOn()  const { return _growLightOn; }

    // True si hace falta atencion relacionada con agua
    // (tierra seca o tanque vacio).
    bool needsWaterAttention() const {
      return _alertSoil || _alertWater;
    }

  private:
    bool _alertSoil;
    bool _alertLight;
    bool _alertWater;
    bool _growLightOn;
    bool _wasLightAlert;
};

#endif
