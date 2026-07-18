#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <string.h>
#include "display.h"

static const int TFT_MOSI = 12, TFT_SCLK = 14, TFT_CS = 1, TFT_DC = 18, TFT_RST = 17;
static SPIClass tftSPI(FSPI);
static Adafruit_ST7735 tft(&tftSPI, TFT_CS, TFT_DC, TFT_RST);

void displayBegin() {
  tftSPI.begin(TFT_SCLK, -1, TFT_MOSI);
  tft.initR(INITR_MINI160x80);
  tft.setRotation(3);
  tft.invertDisplay(true);
  tft.fillScreen(ST77XX_BLACK);
}

void displayLines(const char* const* lines, int count) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  for (int i = 0; i < count && i < 5; i++) {
    tft.setCursor(4, 4 + i * 16);
    tft.print(lines[i]);
  }
}

// "Now playing" card: title/artist + progress bar + play/pause + volume.
// Metadata only (no audio) - drives the transport UI for the Music app.
void displayMusic(const char* title, const char* artist, int pct, bool playing, int vol) {
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(4, 2);
  char t[14];
  size_t tn = strlen(title);
  if (tn > 13) tn = 13;
  memcpy(t, title, tn);
  t[tn] = 0;
  tft.print(t);

  tft.setTextColor(0x07FF);
  tft.setTextSize(1);
  tft.setCursor(4, 24);
  tft.print(artist);

  tft.drawRect(4, 42, 152, 8, 0x8410);
  int fillW = (152 - 2) * pct / 100;
  if (fillW > 0) tft.fillRect(5, 43, fillW, 6, 0x07E0);

  if (playing) {
    tft.fillTriangle(4, 56, 4, 66, 12, 61, ST77XX_WHITE);
  } else {
    tft.fillRect(4, 56, 3, 10, ST77XX_WHITE);
    tft.fillRect(9, 56, 3, 10, ST77XX_WHITE);
  }

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(100, 56);
  char vol_str[16];
  snprintf(vol_str, sizeof(vol_str), "VOL %d", vol);
  tft.print(vol_str);
}

// Draws the bold turn-arrow glyph for the given maneuver, centered on the
// perspective road backdrop. Filled primitives only (fillRect/fillTriangle).
static void drawManeuverArrow(const char* turn) {
  const uint16_t GREEN = 0x07E0;
  if (strcmp(turn, "left") == 0) {
    tft.fillRect(74, 44, 12, 22, GREEN);
    tft.fillRect(48, 44, 32, 12, GREEN);
    tft.fillTriangle(48, 36, 48, 64, 30, 50, GREEN);
  } else if (strcmp(turn, "right") == 0) {
    tft.fillRect(74, 44, 12, 22, GREEN);
    tft.fillRect(80, 44, 32, 12, GREEN);
    tft.fillTriangle(112, 36, 112, 64, 130, 50, GREEN);
  } else if (strcmp(turn, "slight-left") == 0) {
    tft.fillRect(84, 50, 10, 18, GREEN);
    tft.fillTriangle(96, 66, 62, 30, 78, 30, GREEN);
  } else if (strcmp(turn, "slight-right") == 0) {
    tft.fillRect(66, 50, 10, 18, GREEN);
    tft.fillTriangle(64, 66, 98, 30, 82, 30, GREEN);
  } else if (strcmp(turn, "uturn") == 0) {
    tft.fillRect(60, 30, 10, 34, GREEN);
    tft.fillRect(60, 30, 40, 10, GREEN);
    tft.fillRect(90, 30, 10, 24, GREEN);
    tft.fillTriangle(84, 54, 106, 54, 95, 70, GREEN);
  } else {
    // "straight" and any unrecognized turn default to a straight-ahead arrow.
    tft.fillRect(74, 32, 12, 32, GREEN);
    tft.fillTriangle(62, 36, 98, 36, 80, 16, GREEN);
  }
}

// Driver's-eye turn-by-turn view: perspective road, bold turn arrow,
// distance-to-maneuver, and the upcoming street name.
void displayManeuver(const char* turn, long distanceM, const char* street) {
  tft.fillScreen(ST77XX_BLACK);

  // Perspective road backdrop: trapezoid narrowing toward a vanishing point
  // near top-center, built from two filled triangles.
  const uint16_t ROAD = 0x2104;
  const uint16_t EDGE = 0x4208;
  tft.fillTriangle(52, 80, 108, 80, 88, 26, ROAD);
  tft.fillTriangle(52, 80, 88, 26, 72, 26, ROAD);
  tft.drawLine(52, 80, 72, 26, EDGE);
  tft.drawLine(108, 80, 88, 26, EDGE);

  // Dashed center-line receding upward to sell the perspective.
  tft.fillRect(77, 68, 6, 4, ST77XX_WHITE);
  tft.fillRect(78, 56, 4, 3, ST77XX_WHITE);
  tft.fillRect(79, 46, 3, 2, ST77XX_WHITE);

  drawManeuverArrow(turn);

  // Distance-to-maneuver, top-left, bold white.
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(4, 3);
  char dist_str[16];
  if (distanceM < 1000) {
    snprintf(dist_str, sizeof(dist_str), "%ldm", distanceM);
  } else {
    long km = distanceM / 1000;
    long tenths = (distanceM % 1000) / 100;
    snprintf(dist_str, sizeof(dist_str), "%ld.%ldkm", km, tenths);
  }
  tft.print(dist_str);

  // Street name, bottom, cyan.
  tft.setTextColor(0x07FF);
  tft.setTextSize(1);
  tft.setCursor(4, 70);
  char s[27];
  size_t sn = strlen(street);
  if (sn > 26) sn = 26;
  memcpy(s, street, sn);
  s[sn] = 0;
  tft.print(s);
}
