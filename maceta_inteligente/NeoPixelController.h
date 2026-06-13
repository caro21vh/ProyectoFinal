#ifndef NEOPIXEL_CONTROLLER_H
#define NEOPIXEL_CONTROLLER_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

/*
 * NeoPixelController
 * --------------------
 * Encapsula la tira NeoPixel y los 4 estados visuales:
 *
 *  ESTADO 1 - Grow light + parpadeo de aviso de riego/tanque:
 *    growLightOn = true Y needsWaterAttention = true
 *    -> Rosa/magenta con parpadeo ambar cada BLINK_INTERVAL ms.
 *
 *  ESTADO 2 - Solo grow light:
 *    growLightOn = true Y needsWaterAttention = false
 *    -> Rosa/magenta fijo.
 *
 *  ESTADO 3 - Tierra seca (sin problema de luz):
 *    growLightOn = false Y alertSoil = true
 *    -> Parpadeo rojo cada 500ms.
 *
 *  ESTADO 4 - Todo normal:
 *    -> Barra verde proporcional a la humedad del suelo.
 */
class NeoPixelController {
  public:
    static const unsigned long BLINK_DURATION = 250;
    static const unsigned long BLINK_INTERVAL = 3000;

    NeoPixelController(uint8_t pin, uint16_t count)
      : _strip(count, pin, NEO_GRB + NEO_KHZ800), _count(count),
        _lastWaterBlink(0), _redToggle(false), _lastRedToggle(0) {}

    void begin(uint8_t brightness = 80) {
      _strip.begin();
      _strip.setBrightness(brightness);
      _strip.show();
    }

    void update(bool growLightOn, bool alertSoil, bool needsWaterAttention, int soilPercent) {
      if (growLightOn) {
        if (needsWaterAttention) {
          showGrowLightWithBlink();
        } else {
          fill(255, 30, 120);
        }
      } else if (alertSoil) {
        showSoilDryBlink();
      } else {
        showSoilBar(soilPercent);
      }
      _strip.show();
    }

  private:
    Adafruit_NeoPixel _strip;
    uint16_t _count;

    unsigned long _lastWaterBlink;
    bool _redToggle;
    unsigned long _lastRedToggle;

    void fill(uint8_t r, uint8_t g, uint8_t b) {
      for (uint16_t i = 0; i < _count; i++) {
        _strip.setPixelColor(i, _strip.Color(r, g, b));
      }
    }

    void clear() {
      fill(0, 0, 0);
    }

    // ESTADO 1: grow light con parpadeo ambar cada BLINK_INTERVAL ms
    void showGrowLightWithBlink() {
      unsigned long now = millis();
      unsigned long cyclePos = (now - _lastWaterBlink) % BLINK_INTERVAL;

      if (cyclePos < BLINK_DURATION) {
        fill(255, 180, 0); // ambar/amarillo: aviso de regar/tanque
      } else {
        fill(255, 30, 120); // color normal del grow light
      }

      if (now - _lastWaterBlink >= BLINK_INTERVAL) {
        _lastWaterBlink = now;
      }
    }

    // ESTADO 3: parpadeo rojo cada 500ms
    void showSoilDryBlink() {
      if (millis() - _lastRedToggle > 500) {
        _redToggle = !_redToggle;
        _lastRedToggle = millis();
      }
      if (_redToggle) fill(220, 0, 0);
      else clear();
    }

    // ESTADO 4: barra verde proporcional a la humedad
    void showSoilBar(int soilPercent) {
      int ledsOn = map(soilPercent, 0, 100, 0, _count);
      for (uint16_t i = 0; i < _count; i++) {
        _strip.setPixelColor(i, i < ledsOn ? _strip.Color(0, 200, 50) : _strip.Color(0, 0, 0));
      }
    }
};

#endif
