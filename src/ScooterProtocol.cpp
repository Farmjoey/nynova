#include "ScooterProtocol.h"
#include "Diagnostics.h"

namespace ScooterProtocol {

bool parseNotification(const uint8_t* data, size_t len, Telemetry& out) {
  (void)out;  // never touched: no verified format exists
  Diag::logHex("unk", data, len);
  return false;
}

bool requestSpeedLimit(uint8_t kmh) {
  (void)kmh;  // intentionally no I/O
  return false;
}

}  // namespace ScooterProtocol
