#include <Arduino.h>
#include "encoder.h"

static const int PIN_A = 3, PIN_B = 2, PIN_BTN = 4;
static volatile int32_t g_raw = 0;
static volatile uint8_t g_prev = 3;
static const int8_t QTAB[16] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
static long g_lastDet = 0;
static bool g_lastBtn = false;
static uint32_t g_btnMs = 0;

static void IRAM_ATTR isr() {
  uint8_t s = (uint8_t)((digitalRead(PIN_A) << 1) | digitalRead(PIN_B));
  g_raw += QTAB[((g_prev << 2) | s) & 0x0F];
  g_prev = s;
}

void encoderBegin() {
  pinMode(PIN_A, INPUT_PULLUP);
  pinMode(PIN_B, INPUT_PULLUP);
  pinMode(PIN_BTN, INPUT);                 // module pull-down; pressed = HIGH
  g_prev = (uint8_t)((digitalRead(PIN_A) << 1) | digitalRead(PIN_B));
  attachInterrupt(digitalPinToInterrupt(PIN_A), isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_B), isr, CHANGE);
}

int encoderTakeDelta() {
  noInterrupts(); int32_t raw = g_raw; interrupts();
  long det = raw / 4;
  int delta = (int)(det - g_lastDet);
  g_lastDet = det;
  return delta;
}

bool encoderTakePress() {
  bool now = (digitalRead(PIN_BTN) == HIGH);
  bool fired = false;
  if (now != g_lastBtn && (millis() - g_btnMs) > 25) {
    g_btnMs = millis();
    if (now) fired = true;                 // rising edge = press
    g_lastBtn = now;
  }
  return fired;
}

// ---- gesture detector (single / double / long press), active-high on PIN_BTN ----
static bool g_btnState = false; static uint32_t g_lastEdge = 0, g_downAt = 0, g_lastRelease = 0;
static bool g_longFired = false; static uint8_t g_taps = 0;
static const uint32_t GLONG_MS = 600, GDTAP_MS = 280, GDEBOUNCE_MS = 25;

EncoderGesture encoderGesture() {
  uint32_t t = millis();
  bool raw = (digitalRead(PIN_BTN) == HIGH);   // active-high
  EncoderGesture ev = GESTURE_NONE;
  if (raw != g_btnState && (t - g_lastEdge) > GDEBOUNCE_MS) {
    g_lastEdge = t; g_btnState = raw;
    if (raw) { g_downAt = t; g_longFired = false; }          // pressed
    else {                                                    // released
      if (!g_longFired) { g_taps++; g_lastRelease = t; if (g_taps >= 2) { ev = GESTURE_DOUBLE; g_taps = 0; } }
      else g_taps = 0;
    }
  }
  if (ev == GESTURE_NONE && g_btnState && !g_longFired && (t - g_downAt) > GLONG_MS) { g_longFired = true; g_taps = 0; ev = GESTURE_LONG; }
  if (ev == GESTURE_NONE && g_taps == 1 && (t - g_lastRelease) > GDTAP_MS) { g_taps = 0; ev = GESTURE_SINGLE; }
  return ev;
}
