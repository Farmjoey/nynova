#pragma once
// Scooter protocol layer. The E-Wheels e2s Urban Max protocol is NOT verified,
// so this layer deliberately cannot parse telemetry and cannot write anything.
#include <stddef.h>
#include <stdint.h>
#include "Telemetry.h"

namespace ScooterProtocol {
static const bool kVerified = false;
static const bool kWriteSupported = false;

// Always returns false (unknown format). Unknown payloads are only logged.
bool parseNotification(const uint8_t* data, size_t len, Telemetry& out);
// Always returns false. No bytes are ever sent to the scooter.
bool requestSpeedLimit(uint8_t kmh);
}
