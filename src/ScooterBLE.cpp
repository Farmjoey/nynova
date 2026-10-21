#include "ScooterBLE.h"
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <string.h>
#include <ctype.h>
#include "BoardConfig.h"
#include "Diagnostics.h"

namespace {
struct Entry { BleSeen seen; uint32_t lastMs; bool used; };
const uint8_t kMax = 8;
const uint32_t kFreshMs = 30000;
Entry s_tab[kMax];
portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
NimBLEScan* s_scan = nullptr;

bool nameHint(const char* name) {
  char low[20];
  size_t i = 0;
  for (; name[i] && i < sizeof(low) - 1; ++i) low[i] = (char)tolower((unsigned char)name[i]);
  low[i] = 0;
  for (int h = 0; h < board::kScooterNameHintCount; ++h)
    if (strstr(low, board::kScooterNameHints[h])) return true;
  return false;
}

class ScanCb : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice* dev) override {
    BleSeen s;
    strlcpy(s.name, dev->getName().c_str(), sizeof(s.name));
    strlcpy(s.addr, dev->getAddress().toString().c_str(), sizeof(s.addr));
    s.rssi = (int8_t)dev->getRSSI();
    s.candidate = nameHint(s.name);
    const uint32_t now = millis();
    portENTER_CRITICAL(&s_mux);
    int slot = -1, oldest = 0;
    for (int i = 0; i < kMax; ++i) {
      if (s_tab[i].used && strcmp(s_tab[i].seen.addr, s.addr) == 0) { slot = i; break; }
      if (!s_tab[i].used) { if (slot < 0) slot = i; }
      else if (s_tab[i].lastMs < s_tab[oldest].lastMs) oldest = i;
    }
    if (slot < 0) slot = oldest;
    s_tab[slot].seen = s;
    s_tab[slot].lastMs = now;
    s_tab[slot].used = true;
    portEXIT_CRITICAL(&s_mux);
  }
};
ScanCb s_cb;
}  // namespace

void ScooterBLE::setEnabled(bool on) {
  if (on == enabled_) return;
  enabled_ = on;
  if (on) {
    if (!inited_) {
      NimBLEDevice::init("NOVA-RIDE");
      s_scan = NimBLEDevice::getScan();
      s_scan->setAdvertisedDeviceCallbacks(&s_cb, false);
      s_scan->setActiveScan(true);
      s_scan->setInterval(160);
      s_scan->setWindow(80);
      s_scan->setMaxResults(0);  // do not accumulate results on the heap
      inited_ = true;
      Diag::log("BLE scan init (read-only)");
    }
    lastKick_ = 0;
  } else if (inited_ && s_scan) {
    s_scan->stop();
  }
}

void ScooterBLE::loop(uint32_t now) {
  if (!enabled_ || !inited_ || !s_scan) return;
  if (!s_scan->isScanning() && now - lastKick_ >= 500) {
    lastKick_ = now;
    s_scan->clearResults();
    s_scan->start(4, nullptr, false);  // 4 s non-blocking scan, restarted by loop()
  }
}

uint8_t ScooterBLE::count() const {
  const uint32_t now = millis();
  uint8_t n = 0;
  portENTER_CRITICAL(&s_mux);
  for (int i = 0; i < kMax; ++i)
    if (s_tab[i].used && now - s_tab[i].lastMs < kFreshMs) ++n;
  portEXIT_CRITICAL(&s_mux);
  return n;
}

bool ScooterBLE::get(uint8_t idx, BleSeen& out) const {
  const uint32_t now = millis();
  bool found = false;
  uint8_t n = 0;
  portENTER_CRITICAL(&s_mux);
  for (int i = 0; i < kMax; ++i) {
    if (!s_tab[i].used || now - s_tab[i].lastMs >= kFreshMs) continue;
    if (n++ == idx) { out = s_tab[i].seen; found = true; break; }
  }
  portEXIT_CRITICAL(&s_mux);
  return found;
}

BleState ScooterBLE::state() const {
  if (!enabled_) return BleState::Off;
  const uint32_t now = millis();
  bool cand = false;
  portENTER_CRITICAL(&s_mux);
  for (int i = 0; i < kMax; ++i)
    if (s_tab[i].used && s_tab[i].seen.candidate && now - s_tab[i].lastMs < kFreshMs) cand = true;
  portEXIT_CRITICAL(&s_mux);
  return cand ? BleState::CandidateSeen : BleState::Scanning;
}
