#ifndef SENSOR_READER_H
#define SENSOR_READER_H

#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>

/*
 * SensorReader
 * -------------
 * Encapsula la lectura de los sensores fisicos:
 *  - Higrometro FC-28 (humedad de suelo, analogico)
 *  - Sensor de nivel de agua (analogico)
 *  - BH1750 (luz ambiental, I2C)
 */
class SensorReader {
  public:
    SensorReader(uint8_t soilPin, uint8_t waterPin)
      : _soilPin(soilPin), _waterPin(waterPin),
        _soilRaw(0), _soilPercent(0), _waterRaw(0), _lightLux(0.0) {}

    void begin() {
      Wire.begin(21, 22);
      _lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
    }

    // Lee todos los sensores y actualiza los valores internos
    void read() {
      if (_lightMeter.measurementReady()) {
        _lightLux = _lightMeter.readLightLevel();
      }
      _soilRaw     = analogRead(_soilPin);
      _soilPercent = constrain(map(_soilRaw, 4095, 0, 0, 100), 0, 100);
      _waterRaw    = analogRead(_waterPin);
    }

    int   getSoilRaw()     const { return _soilRaw; }
    int   getSoilPercent() const { return _soilPercent; }
    int   getWaterRaw()    const { return _waterRaw; }
    float getLightLux()    const { return _lightLux; }

    void printDebug() const {
      Serial.printf("Humedad: %d%% (raw:%d) | Luz: %.1f lux | Agua tanque raw: %d\n",
                     _soilPercent, _soilRaw, _lightLux, _waterRaw);
    }

  private:
    uint8_t _soilPin;
    uint8_t _waterPin;
    BH1750  _lightMeter;

    int   _soilRaw;
    int   _soilPercent;
    int   _waterRaw;
    float _lightLux;
};

#endif
