#include "display.h"
#include "encoder.h"
#include "line_framer.h"
#include "ble_link.h"
#include "apps.h"

static void onPhoneMessage(const char* json) {
  Serial.printf("rx: %s\n", json);
  // Echo: reply hat.ready to a ping so we can confirm bidirectional flow.
  if (strstr(json, "\"ping\"")) {
    bleSend("{\"type\":\"pong\"}");
    return;
  }
  appsMessage(json);
  char msg[48]; snprintf(msg, sizeof(msg), "{\"type\":\"app.changed\",\"payload\":{\"id\":\"%s\"}}", appsActiveId());
  bleSend(msg);
}

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
  bleBegin(onPhoneMessage);
  appsBegin();
  const char* boot[] = {"HatOS", "ready"};
  displayLines(boot, 2);
  runFramerSelfTest();
  runOverflowSelfTest();
}

static bool g_wasConnected = false;

void loop() {
  // hat.ready on new connection
  bool c = bleConnected();
  if (c && !g_wasConnected) {
    char cat[192]; appsRenderCatalogJson(cat, sizeof(cat));
    char msg[256];
    snprintf(msg, sizeof(msg),
      "{\"type\":\"hat.ready\",\"payload\":{\"name\":\"HatOS\",\"firmware\":\"foundation-0.1\",\"apps\":%s}}", cat);
    bleSend(msg);
  }
  g_wasConnected = c;

  int d = encoderTakeDelta();
  if (d != 0) {
    appsEncoder(d);
    char msg[64];
    snprintf(msg, sizeof(msg), "{\"type\":\"encoder\",\"payload\":{\"delta\":%d,\"pressed\":false}}", d);
    bleSend(msg);
  }
  if (encoderTakePress()) {
    appsButton();
    bleSend("{\"type\":\"encoder\",\"payload\":{\"delta\":0,\"pressed\":true}}");
    char msg[48]; snprintf(msg, sizeof(msg), "{\"type\":\"app.changed\",\"payload\":{\"id\":\"%s\"}}", appsActiveId());
    bleSend(msg);
  }
  delay(5);
}
