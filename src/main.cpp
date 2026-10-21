// NOVA RIDE OS - firmware entry point.
#include <Arduino.h>
#include "BoardConfig.h"
#include "Diagnostics.h"
#include "SettingsStorage.h"
#include "TouchInput.h"
#include "ScooterBLE.h"
#include "PhoneMedia.h"
#include "Notifications.h"
#include "SafetyProfile.h"
#include "DemoTelemetry.h"
#include "DisplayUI.h"

static SettingsStorage g_store;
static TouchInput g_touch;
static BootButton g_boot;
static ScooterBLE g_ble;
static PhoneMedia g_phone;
static Notifications g_notes;
static SafetyProfile g_safety;
static DisplayUI g_ui;

void setup() {
  Serial.begin(115200);
  Diag::log("NOVA RIDE OS v%s [%s]", NOVA_FW_VERSION, NOVA_PROFILE_NAME);
  g_store.begin();
  g_touch.begin();
  g_boot.begin();
  g_ble.begin();
  g_phone.begin();
  g_notes.begin();

  UiContext ctx;
  ctx.store = &g_store;
  ctx.ble = &g_ble;
  ctx.phone = &g_phone;
  ctx.notes = &g_notes;
  ctx.safety = &g_safety;
  g_ui.begin(ctx);

  if (g_store.get().bleScan) g_ble.setEnabled(true);
}

void loop() {
  const uint32_t now = millis();
  const TouchState ts = g_touch.poll();
  const int b = g_boot.poll(now);
  if (b) g_ui.onBootButton(b == 2);

  g_ble.loop(now);
  g_phone.loop(now);
  g_notes.loop(now);

  // Telemetry source: labelled demo data, or "no data" (all invalid). Never invented.
  Telemetry t;
  const Settings& s = g_store.get();
  if (s.demo) t = demoSample(now, g_safety.demoArmed() ? kQuickLimitKmh : 0);
  if (!s.demo) g_safety.clearDemo();

  g_ui.update(t, ts, now);
  delay(4);  // yields to FreeRTOS idle task (watchdog-safe)
}
