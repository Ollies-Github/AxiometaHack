// Discovery firmware:
//  1) Drives the 0.96" ST7735 TFT (Port 4 pins) with a color/test pattern so we
//     can confirm the display + DC/RST mapping + color inversion.
//  2) Watches EVERY AX22 port-unique GPIO (all 4 ports) and reports which pins
//     toggle when the encoder is turned/pressed -> finds the encoder's real port.
//
// Display pins (Port 4 + shared SPI):
//   MOSI=12, SCK=14, CS=17, DC=18, RST=1   (DC/RST swapped vs first try)

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

constexpr int TFT_MOSI = 12, TFT_SCLK = 14, TFT_CS = 17, TFT_DC = 18, TFT_RST = 1;
SPIClass tftSPI(FSPI);
Adafruit_ST7735 tft(&tftSPI, TFT_CS, TFT_DC, TFT_RST);

// All AX22 port-unique GPIOs EXCEPT the 3 used by the display (1,17,18):
//   Port1: 4,3,2   Port2: 7,6,5   Port3: 9,16,15
const int scanPins[] = {2, 3, 4, 5, 6, 7, 9, 15, 16};
const int N = sizeof(scanPins) / sizeof(scanPins[0]);
int lastVal[N];
long edgeCount[N] = {0};

void drawHeader() {
  tft.fillScreen(ST77XX_BLACK);
  // color bars to check color + inversion
  tft.fillRect(0, 0, 20, 20, ST77XX_RED);
  tft.fillRect(20, 0, 20, 20, ST77XX_GREEN);
  tft.fillRect(40, 0, 20, 20, ST77XX_BLUE);
  tft.fillRect(60, 0, 20, 20, ST77XX_WHITE);
  tft.setTextColor(ST77XX_YELLOW); tft.setTextSize(1);
  tft.setCursor(84, 2);  tft.print("R G B W");
  tft.setCursor(84, 12); tft.print("colors ok?");
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(2, 26); tft.print("TURN / PRESS the knob");
  tft.drawFastHLine(0, 36, 160, 0x4208);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n== DISCOVERY: find encoder pins ==");

  tftSPI.begin(TFT_SCLK, -1, TFT_MOSI);
  tft.initR(INITR_MINI160x80);
  tft.setRotation(3);
  tft.invertDisplay(true);
  drawHeader();

  for (int i = 0; i < N; i++) {
    pinMode(scanPins[i], INPUT_PULLUP);
    lastVal[i] = digitalRead(scanPins[i]);
  }
  Serial.print("Watching GPIOs:");
  for (int i = 0; i < N; i++) Serial.printf(" %d", scanPins[i]);
  Serial.println();
}

void redrawResults() {
  tft.fillRect(0, 40, 160, 40, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(2, 40); tft.setTextColor(ST77XX_WHITE);
  tft.print("active GPIOs:");
  int x = 2, y = 52;
  for (int i = 0; i < N; i++) {
    if (edgeCount[i] > 0) {
      tft.setTextColor(ST77XX_GREEN);
      tft.setCursor(x, y);
      tft.printf("%d", scanPins[i]);
      x += 24;
      if (x > 150) { x = 2; y += 12; }
    }
  }
}

void loop() {
  bool changed = false;
  for (int i = 0; i < N; i++) {
    int v = digitalRead(scanPins[i]);
    if (v != lastVal[i]) {
      lastVal[i] = v;
      edgeCount[i]++;
      changed = true;
      Serial.printf("GPIO%-2d -> %d   (edges: %ld)\n", scanPins[i], v, edgeCount[i]);
    }
  }
  if (changed) redrawResults();
  delay(2);
}
