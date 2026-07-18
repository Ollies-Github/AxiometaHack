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
