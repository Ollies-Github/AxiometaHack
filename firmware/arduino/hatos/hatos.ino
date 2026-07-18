#include "display.h"
#include "encoder.h"

static int counter = 0;

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n== HatOS foundation ==");
  displayBegin();
  encoderBegin();
  const char* boot[] = {"HatOS", "ready"};
  displayLines(boot, 2);
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
