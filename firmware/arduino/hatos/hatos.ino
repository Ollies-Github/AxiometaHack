#include "display.h"
#include "encoder.h"
#include "line_framer.h"

static int counter = 0;

static int g_selfTestLines = 0;
static void selfTestSink(const char* line) { g_selfTestLines++; }

static void runFramerSelfTest() {
  LineFramer f;
  const char* s = "abc\n{\"type\":\"ping\"}\npartial";
  f.push((const uint8_t*)s, strlen(s), selfTestSink);
  Serial.printf("framer self-test: %s (%d lines, expected 2)\n",
                g_selfTestLines == 2 ? "PASS" : "FAIL", g_selfTestLines);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n== HatOS foundation ==");
  displayBegin();
  encoderBegin();
  const char* boot[] = {"HatOS", "ready"};
  displayLines(boot, 2);
  runFramerSelfTest();
}

void loop() {
  int d = encoderTakeDelta();
  bool p = encoderTakePress();
  if (d != 0 || p) {
    if (p) counter = 0; else counter += d;
    Serial.printf("delta=%d press=%d counter=%d\n", d, p, counter);
    char line0[24]; snprintf(line0, sizeof(line0), "count %d", counter);
    const char* lines[] = {"encoder", line0};
    displayLines(lines, 2);
  }
  delay(5);
}
