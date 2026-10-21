#pragma once
#include <stdint.h>
#include "TouchCal.h"

struct Settings {
  bool demo;            // demo/test telemetry (default ON until real protocol exists)
  bool bleScan;         // passive BLE scan list
  bool invert;          // panel colour inversion
  uint8_t rotation;     // 1 or 3 (landscape)
  uint8_t backlightPct; // 10..100
  bool hwConfirmed;     // user accepted the hardware test for this profile
  Settings() : demo(true), bleScan(false), invert(false), rotation(1), backlightPct(80), hwConfirmed(false) {}
};

class SettingsStorage {
 public:
  void begin();
  Settings& get() { return s_; }
  void save();
  bool loadCal(uint8_t rotation, TouchCal& out);
  void saveCal(uint8_t rotation, const TouchCal& cal);
 private:
  Settings s_;
};
