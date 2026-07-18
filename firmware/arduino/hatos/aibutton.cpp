#include <Arduino.h>
#include "aibutton.h"

// Verified on hardware (Port 3 probe): the button is on IO1 = GPIO16 and is
// active-HIGH (idle LOW via an on-board pull-down, pressed HIGH — same as the
// encoder button). The LED sits on one of the other two Port-3 pins (IO0=GPIO9
// or IO2=GPIO15); we drive both together so it lights regardless of which the
// module used. An LED is passive, so this doesn't disturb the shared SPI bus
// the way the vibration motor did.
static const int BTN = 16;
static const int LED_A = 9, LED_B = 15;
static const uint32_t LED_TIMEOUT_MS = 8000;

static bool g_pressed = false;
static bool g_ledOn = false;
static uint32_t g_ledDeadline = 0;
static uint32_t g_lastEdge = 0;

void aiButtonBegin() {
  pinMode(BTN, INPUT);            // on-board pull-down; pressed reads HIGH
  pinMode(LED_A, OUTPUT); pinMode(LED_B, OUTPUT);
  digitalWrite(LED_A, LOW); digitalWrite(LED_B, LOW);
  g_pressed = false; g_ledOn = false;
}

void aiLedSet(bool on) {
  g_ledOn = on;
  digitalWrite(LED_A, on ? HIGH : LOW);
  digitalWrite(LED_B, on ? HIGH : LOW);
  if (on) g_ledDeadline = millis() + LED_TIMEOUT_MS;
}

int aiButtonPoll() {
  bool raw = digitalRead(BTN) == HIGH;
  uint32_t now = millis();
  if (raw != g_pressed && (now - g_lastEdge) > 30) {   // 30 ms debounce
    g_pressed = raw;
    g_lastEdge = now;
    return raw ? +1 : -1;
  }
  return 0;
}

void aiLedTick() {
  if (g_ledOn && (int32_t)(millis() - g_ledDeadline) > 0) aiLedSet(false);
}
