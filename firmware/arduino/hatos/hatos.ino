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

static int g_overflowTestLines = 0;
static bool g_overflowTestLineOk = false;
static void overflowTestSink(const char* line) {
  g_overflowTestLines++;
  g_overflowTestLineOk = (strcmp(line, "ok") == 0);
}

static void runOverflowSelfTest() {
  LineFramer f;
  // Build a single line longer than the 240-char buffer, followed by a
  // valid short line. The oversized line must be fully dropped: no
  // phantom fragment should reach onLine, and the only emitted line
  // should be "ok".
  String s;
  for (int i = 0; i < 300; i++) s += 'x';
  s += "\nok\n";
  f.push((const uint8_t*)s.c_str(), s.length(), overflowTestSink);
  bool pass = (g_overflowTestLines == 1) && g_overflowTestLineOk;
  Serial.printf("overflow self-test: %s (%d lines, expected 1)\n",
                pass ? "PASS" : "FAIL", g_overflowTestLines);
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
  runOverflowSelfTest();
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
