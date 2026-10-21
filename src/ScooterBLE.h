#pragma once
// Read-only BLE scanner. It only LISTS advertising devices. It never connects,
// never writes, and never sends commands: the scooter protocol is unverified.
#include <stdint.h>

enum class BleState : uint8_t { Off, Scanning, CandidateSeen };

struct BleSeen {
  char name[20];
  char addr[18];
  int8_t rssi;
  bool candidate;  // name matches a hint - NOT a verified identification
  BleSeen() : rssi(0), candidate(false) { name[0] = 0; addr[0] = 0; }
};

class ScooterBLE {
 public:
  ScooterBLE() : enabled_(false), inited_(false), lastKick_(0) {}
  void begin() {}
  void setEnabled(bool on);
  bool enabled() const { return enabled_; }
  void loop(uint32_t now);
  BleState state() const;
  uint8_t count() const;              // devices seen in the last 30 s
  bool get(uint8_t idx, BleSeen& out) const;
 private:
  bool enabled_, inited_;
  uint32_t lastKick_;
};
