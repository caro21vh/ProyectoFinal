#ifndef BUZZER_CONTROLLER_H
#define BUZZER_CONTROLLER_H

#include <Arduino.h>

/*
 * BuzzerController
 * -----------------
 * Encapsula el buzzer: todos los patrones de sonido (beeps)
 * y el control de tiempos para no repetir alertas demasiado
 * seguido (debounce por millis()).
 */
class BuzzerController {
  public:
    BuzzerController(uint8_t pin, unsigned long repeatIntervalMs = 5000)
      : _pin(pin), _repeatIntervalMs(repeatIntervalMs),
        _lastTankBeep(0), _lastSoilBeep(0) {}

    void begin() {
      pinMode(_pin, OUTPUT);
      digitalWrite(_pin, LOW);
      ledcSetup(0, 5000, 8);
      ledcAttachPin(_pin, 0);
    }

    // Debe llamarse en cada ciclo de loop con los estados actuales
    // de alerta; internamente decide si toca sonar o no.
    void update(bool alertWater, bool alertSoil, bool lightJustRecovered) {
      unsigned long now = millis();

      if (alertWater && (now - _lastTankBeep > _repeatIntervalMs)) {
        beepTanqueVacio();
        _lastTankBeep = now;
      }

      if (alertSoil && (now - _lastSoilBeep > _repeatIntervalMs)) {
        beepTierraSeca();
        _lastSoilBeep = now;
      }

      if (lightJustRecovered) {
        beepLuzOk();
      }
    }

    void beepWifiConnected() {
      tone(800, 80);  silence(40);
      tone(1200, 80); silence(40);
      tone(1800, 120); silence(0);
    }

    void beepTanqueVacio() {
      tone(400, 200); silence(100);
      tone(400, 200); silence(0);
    }

    void beepTierraSeca() {
      tone(900, 120); silence(80);
      tone(900, 120); silence(0);
    }

    void beepLuzOk() {
      tone(1000, 60);
      tone(1400, 60);
      tone(1800, 100); silence(0);
    }

    void beepNotification() {
      tone(1000, 80);
      tone(1500, 120); silence(0);
    }

  private:
    uint8_t _pin;
    unsigned long _repeatIntervalMs;
    unsigned long _lastTankBeep;
    unsigned long _lastSoilBeep;

    void tone(int freq, int durationMs) {
      ledcWriteTone(0, freq);
      delay(durationMs);
    }

    void silence(int durationMs) {
      ledcWriteTone(0, 0);
      if (durationMs > 0) delay(durationMs);
    }
};

#endif
