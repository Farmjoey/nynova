// Host-side unit tests for the pure logic modules (no ESP32 needed).
#include <stdio.h>
#include <string.h>
#include "../../src/Format.h"
#include "../../src/Gesture.h"
#include "../../src/TouchCal.h"
#include "../../src/SafetyLogic.h"
#include "../../src/SafetyProfile.h"
#include "../../src/DemoTelemetry.h"

static int g_fail = 0, g_n = 0;
#define CHECK(c) do { ++g_n; if (!(c)) { ++g_fail; printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)

// Stubs for the two functions the real firmware implements in ScooterProtocol.cpp
namespace ScooterProtocol {
bool parseNotification(const uint8_t*, size_t, Telemetry&) { return false; }
bool requestSpeedLimit(uint8_t) { return false; }
}

static GestureEvent drag(GestureDetector& g, int x0, int y0, int x1, int y1, uint32_t ms) {
  uint32_t t = 1000;
  g.update(true, (int16_t)x0, (int16_t)y0, t);
  g.update(true, (int16_t)((x0 + x1) / 2), (int16_t)((y0 + y1) / 2), t + ms / 2);
  g.update(true, (int16_t)x1, (int16_t)y1, t + ms - 1);
  return g.update(false, 0, 0, t + ms);
}

static void test_format() {
  char b[16];
  fmtFixed(b, sizeof b, false, 12.3f, 1); CHECK(strcmp(b, "--") == 0);   // never invent values
  fmtFixed(b, sizeof b, true, 24.6f, 0);  CHECK(strcmp(b, "25") == 0);
  fmtFixed(b, sizeof b, true, 37.24f, 1); CHECK(strcmp(b, "37.2") == 0);
}

static void test_telemetry_default_invalid() {
  Telemetry t;
  CHECK(!t.speedValid && !t.battPctValid && !t.voltValid && !t.powerValid);
  CHECK(!t.tripValid && !t.totalValid && !t.faultValid && !t.modeValid && !t.isDemo);
}

static void test_gestures() {
  GestureDetector g;
  GestureEvent e = drag(g, 160, 180, 165, 90, 250);
  CHECK(e.type == GestureType::SwipeUp);
  e = drag(g, 160, 80, 158, 170, 250);   CHECK(e.type == GestureType::SwipeDown);
  e = drag(g, 250, 100, 100, 110, 250);  CHECK(e.type == GestureType::SwipeLeft);
  e = drag(g, 100, 100, 250, 105, 250);  CHECK(e.type == GestureType::SwipeRight);
  e = drag(g, 100, 100, 104, 103, 120);  CHECK(e.type == GestureType::Tap);
  CHECK(e.x == 100 && e.y == 100);
  e = drag(g, 160, 180, 165, 90, 1500);  CHECK(e.type == GestureType::None);   // too slow
  e = drag(g, 160, 180, 195, 150, 250);  CHECK(e.type == GestureType::None);   // diagonal / short
  e = g.update(false, 0, 0, 5000);       CHECK(e.type == GestureType::None);   // release w/o press
}

static void test_hold() {
  HoldTracker h;
  CHECK(!h.update(true, 1000, 1500));
  CHECK(!h.update(true, 2000, 1500));
  CHECK(h.update(true, 2500, 1500));     // fires once
  CHECK(!h.update(true, 2600, 1500));
  CHECK(!h.update(false, 2700, 1500));   // released
  CHECK(!h.update(true, 2800, 1500));    // restarts
  CHECK(h.progress(3300, 1500) > 0.3f && h.progress(3300, 1500) < 0.4f);
  h.update(false, 3400, 1500);
  CHECK(h.progress(3500, 1500) == 0.0f);
}

static RawPt rawFromScreen(float sx, float sy, bool swap, bool flipX, bool flipY) {
  float a = flipX ? (3800.0f - sx * 11.0f) : (200.0f + sx * 11.0f);   // along screen X
  float b = flipY ? (3800.0f - sy * 15.0f) : (200.0f + sy * 15.0f);   // along screen Y
  RawPt p;
  if (swap) { p.x = (int16_t)b; p.y = (int16_t)a; } else { p.x = (int16_t)a; p.y = (int16_t)b; }
  return p;
}

static void test_cal_variant(bool swap, bool fx, bool fy) {
  TouchCal c;
  RawPt tl = rawFromScreen(20, 20, swap, fx, fy);
  RawPt tr = rawFromScreen(300, 20, swap, fx, fy);
  RawPt br = rawFromScreen(300, 220, swap, fx, fy);
  CHECK(TouchCal::compute(tl, tr, br, 320, 240, 20, c));
  CHECK(c.swapXY == swap);
  const float pts[4][2] = {{20, 20}, {300, 220}, {160, 120}, {60, 200}};
  for (int i = 0; i < 4; ++i) {
    RawPt r = rawFromScreen(pts[i][0], pts[i][1], swap, fx, fy);
    int16_t sx, sy; c.map(r.x, r.y, 320, 240, sx, sy);
    CHECK(abs(sx - (int)pts[i][0]) <= 2 && abs(sy - (int)pts[i][1]) <= 2);
  }
  int16_t sx, sy; c.map(0, 0, 320, 240, sx, sy);
  CHECK(sx >= 0 && sx < 320 && sy >= 0 && sy < 240);   // clamped
}

static void test_cal() {
  for (int s = 0; s < 2; ++s) for (int x = 0; x < 2; ++x) for (int y = 0; y < 2; ++y)
    test_cal_variant(s != 0, x != 0, y != 0);
  TouchCal bad; RawPt same = {1000, 1000};
  CHECK(!TouchCal::compute(same, same, same, 320, 240, 20, bad));  // implausible input rejected
  CHECK(!bad.valid);
}

static void test_safety() {
  SafetyInputs in;
  in.protocolVerified = false; in.linkUp = true; in.ackReceived = true; in.demoMode = false;
  CHECK(computeLimitStatus(in) == LimitStatus::NotSupported);   // never Confirmed w/o verified protocol
  in.demoMode = true;
  CHECK(computeLimitStatus(in) == LimitStatus::DemoSimulated);
  in.protocolVerified = true; in.linkUp = false; in.ackReceived = false;
  CHECK(computeLimitStatus(in) == LimitStatus::Unknown);        // link error -> unknown
  in.linkUp = true;
  CHECK(computeLimitStatus(in) == LimitStatus::Unknown);        // no ack -> unknown
  in.ackReceived = true;
  CHECK(computeLimitStatus(in) == LimitStatus::Confirmed);

  CHECK(mayRequestLimit(20, false, 0));
  CHECK(mayRequestLimit(20, true, 25));
  CHECK(mayRequestLimit(20, true, 20));
  CHECK(!mayRequestLimit(20, true, 15));   // would raise the limit
  CHECK(!mayRequestLimit(25, true, 30));   // only 20 allowed
  CHECK(!mayRequestLimit(0, false, 0));

  SafetyProfile sp;
  CHECK(sp.status(false) == LimitStatus::NotSupported);
  CHECK(sp.requestQuickLimit(false) == QuickResult::NotSupported && !sp.demoArmed());
  CHECK(sp.status(true) == LimitStatus::DemoSimulated);
  CHECK(sp.requestQuickLimit(true) == QuickResult::DemoOnly && sp.demoArmed());
  sp.clearDemo(); CHECK(!sp.demoArmed());
  sp.requestQuickLimit(true);
  CHECK(sp.requestQuickLimit(false) == QuickResult::NotSupported && !sp.demoArmed());
  CHECK(!ScooterProtocol::kVerified && !ScooterProtocol::kWriteSupported);
  CHECK(!ScooterProtocol::requestSpeedLimit(20));                // nothing is ever sent
}

static void test_demo() {
  for (uint32_t t = 0; t < 600000; t += 777) {
    Telemetry d = demoSample(t, 0);
    CHECK(d.isDemo && d.speedValid && d.speedKmh >= 0.0f && d.speedKmh <= 31.0f);
    Telemetry c = demoSample(t, 20);
    CHECK(c.speedKmh <= 20.001f && c.isDemo);                    // demo cap works
    CHECK(c.battPct >= 30 && c.battPct <= 100);
  }
}

int main() {
  test_format(); test_telemetry_default_invalid(); test_gestures(); test_hold();
  test_cal(); test_safety(); test_demo();
  printf("%d checks, %d failed\n", g_n, g_fail);
  return g_fail ? 1 : 0;
}
