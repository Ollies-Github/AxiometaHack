// Pure GPIO scanner (no display). Watches ALL AX22 port-unique GPIOs across all
// four ports and reports which toggle. Finds the encoder regardless of which
// physical port it's in.
//   Port1: 2,3,4   Port2: 5,6,7   Port3: 9,15,16   Port4: 1,17,18
const int pins[] = {1, 2, 3, 4, 5, 6, 7, 9, 15, 16, 17, 18};
const int N = sizeof(pins) / sizeof(pins[0]);
int lastVal[N];
long edges[N] = {0};
uint32_t lastBeat = 0;

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n== PINSCAN: all AX22 port GPIOs ==");
  for (int i = 0; i < N; i++) {
    pinMode(pins[i], INPUT_PULLUP);
    lastVal[i] = digitalRead(pins[i]);
  }
  Serial.print("baseline:");
  for (int i = 0; i < N; i++) Serial.printf(" G%d=%d", pins[i], lastVal[i]);
  Serial.println();
}

void loop() {
  for (int i = 0; i < N; i++) {
    int v = digitalRead(pins[i]);
    if (v != lastVal[i]) {
      lastVal[i] = v;
      edges[i]++;
      Serial.printf("GPIO%-2d -> %d   (edges %ld)\n", pins[i], v, edges[i]);
    }
  }
  if (millis() - lastBeat > 3000) {   // heartbeat summary
    lastBeat = millis();
    Serial.print("edges:");
    for (int i = 0; i < N; i++) if (edges[i]) Serial.printf(" G%d=%ld", pins[i], edges[i]);
    Serial.println();
  }
  delay(1);
}
