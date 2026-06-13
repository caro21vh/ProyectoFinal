#ifndef PLANT_PROFILE_H
#define PLANT_PROFILE_H

#include <Arduino.h>

/*
 * PlantProfile
 * -------------
 * Representa los umbrales de una planta y ofrece metodos
 * para evaluar si una lectura indica "tierra seca" o
 * "luz insuficiente" segun ese perfil.
 *
 * soilDryThreshold: valor RAW del higrometro a partir del cual
 *   se considera "tierra seca" (MAS ALTO = tolera mas sequedad).
 * lightMinLux: luz minima requerida (MAS ALTO = necesita mas luz).
 */
class PlantProfile {
  public:
    PlantProfile(const String& name, int soilDryThreshold, float lightMinLux)
      : _name(name), _soilDryThreshold(soilDryThreshold), _lightMinLux(lightMinLux) {}

    const String& getName() const { return _name; }
    int   getSoilDryThreshold() const { return _soilDryThreshold; }
    float getLightMinLux()      const { return _lightMinLux; }

    bool isSoilDry(int soilRaw) const {
      return soilRaw > _soilDryThreshold;
    }

    bool isLightLow(float lightLux) const {
      return lightLux < _lightMinLux;
    }

  private:
    String _name;
    int    _soilDryThreshold;
    float  _lightMinLux;
};

#endif
