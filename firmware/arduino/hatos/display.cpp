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

// "Now playing" card: album art LEFT (64x64 at x0..63,y8..71), text/controls
// RIGHT. Split into a static part (redrawn only on track change), the art
// bitmap, a scrolling title marquee, and a dynamic part (progress bar /
// play-pause / volume, repainted often without a fillScreen so smooth
// interpolated progress doesn't flicker).
void displayMusicArt(const uint16_t* art, bool has) {
  if (has) tft.drawRGBBitmap(0, 8, art, 64, 64);
  else     tft.fillRect(0, 8, 64, 64, ST77XX_BLACK);
}

// Full non-art chrome: clears screen, draws artist + progress-bar outline
// (title itself is drawn separately by the marquee helper below).
void displayMusicStatic(const char* artist) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(0x07FF);
  tft.setTextSize(1);
  tft.setCursor(68, 18);
  tft.print(artist);
  tft.drawRect(68, 40, 90, 7, 0x8410);  // progress bar outline (static)
}

// Marquee title in the strip x64..159. Clips per-character so it never
// overwrites the album art at x0..63 (strip clear also starts at x64).
void displayMusicTitle(const char* title, int scroll) {
  const int X0 = 68, X1 = 159, GAP = 26, CW = 6;
  tft.fillRect(64, 2, 96, 10, ST77XX_BLACK);  // clear strip (right of art)
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  int tw = (int)strlen(title) * CW;
  if (tw <= (X1 - X0)) {
    tft.setCursor(X0, 3);
    tft.print(title);
    return;
  }
  int period = tw + GAP;
  int so = scroll % period;
  for (int pass = 0; pass < 2; pass++) {  // two copies for seamless wrap
    int base = X0 - so + pass * period;
    for (int i = 0; title[i]; i++) {
      int x = base + i * CW;
      if (x + CW >= X0 && x <= X1) {
        tft.drawChar(x, 3, title[i], ST77XX_WHITE, ST77XX_BLACK, 1);
      }
    }
  }
}

void displayMusicProgress(int pct, bool playing, int vol) {
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;

  tft.fillRect(69, 41, 88, 5, ST77XX_BLACK);  // clear inside of bar
  int fillW = 88 * pct / 100;
  if (fillW > 0) tft.fillRect(69, 41, fillW, 5, 0x07E0);

  tft.fillRect(66, 50, 16, 14, ST77XX_BLACK);  // clear icon area
  if (playing) {
    tft.fillTriangle(68, 52, 68, 62, 76, 57, ST77XX_WHITE);
  } else {
    tft.fillRect(68, 52, 3, 10, ST77XX_WHITE);
    tft.fillRect(73, 52, 3, 10, ST77XX_WHITE);
  }

  tft.fillRect(106, 50, 54, 12, ST77XX_BLACK);  // clear vol text
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(110, 52);
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
