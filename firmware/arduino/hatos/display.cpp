#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <string.h>
#include "display.h"

static const int TFT_MOSI = 12, TFT_SCLK = 14, TFT_CS = 1, TFT_DC = 18, TFT_RST = 17;
static SPIClass tftSPI(FSPI);
static Adafruit_ST7735 tft(&tftSPI, TFT_CS, TFT_DC, TFT_RST);

// ---- Theme -----------------------------------------------------------------
// Dark is the default; light is a warm off-white. Base colours are swapped
// here; per-app accents pick deeper tones on light (see appAccent below) so
// neon colours don't wash out on the pale background.
static bool g_light = false;
static uint16_t COL_BG, COL_INK, COL_DIM, COL_FAINT, COL_PRIMARY;
static void applyTheme() {
  if (g_light) {
    COL_BG = 0xF79C; COL_INK = 0x2124; COL_DIM = 0x7BCF; COL_FAINT = 0xC618; COL_PRIMARY = 0x4CC5;
  } else {
    COL_BG = 0x0000; COL_INK = 0xFFFF; COL_DIM = 0x8410; COL_FAINT = 0x39C7; COL_PRIMARY = 0xBFEC;
  }
}
void displaySetTheme(bool light) { g_light = light; applyTheme(); }
bool displayThemeLight() { return g_light; }

void displayBegin() {
  tftSPI.begin(TFT_SCLK, -1, TFT_MOSI);
  tft.initR(INITR_MINI160x80);
  tft.setRotation(3);
  tft.invertDisplay(true);
  applyTheme();
  tft.fillScreen(COL_BG);
}

void displayLines(const char* const* lines, int count) {
  tft.fillScreen(COL_BG);
  tft.setTextColor(COL_INK);
  tft.setTextSize(2);
  for (int i = 0; i < count && i < 5; i++) {
    tft.setCursor(4, 4 + i * 16);
    tft.print(lines[i]);
  }
}

// ---- Shared visual identity ------------------------------------------------
// Per-app accent: bright neons on dark, deeper tones on the off-white light
// theme so they keep enough contrast.
static uint16_t appAccent(const char* id) {
  if (g_light) {
    if (!strcmp(id, "counter")) return 0xDC01;   // deep amber
    if (!strcmp(id, "notes"))   return 0x4CC5;   // forest green
    if (!strcmp(id, "music"))   return 0x0BB2;   // teal
    if (!strcmp(id, "maps"))    return 0x695B;   // violet
    return 0x0BB2;
  }
  if (!strcmp(id, "counter")) return 0xFE6B;     // amber
  if (!strcmp(id, "notes"))   return 0xBFEC;     // lime
  if (!strcmp(id, "music"))   return 0x675F;     // cyan
  if (!strcmp(id, "maps"))    return 0xA45F;     // violet
  return 0x675F;
}
static const char* appBlurb(const char* id) {
  if (!strcmp(id, "counter")) return "Demo dial";
  if (!strcmp(id, "notes"))   return "Teleprompter";
  if (!strcmp(id, "music"))   return "Now playing";
  if (!strcmp(id, "maps"))    return "Turn-by-turn";
  return "";
}

// Centered helper for a size-1/size-2 label.
static void printCentered(const char* s, int cx, int y, int size, uint16_t col) {
  int w = (int)strlen(s) * 6 * size;
  tft.setTextSize(size);
  tft.setTextColor(col);
  tft.setCursor(cx - w / 2, y);
  tft.print(s);
}

// A 3-bar WiFi signal glyph at (x,y); accent when connected, faint when not.
static void drawWifi(int x, int y, bool on) {
  uint16_t c = on ? COL_PRIMARY : COL_FAINT;
  tft.fillRect(x,     y + 7, 3, 4,  c);
  tft.fillRect(x + 5, y + 4, 3, 7,  c);
  tft.fillRect(x + 10, y + 1, 3, 10, c);
}

// The HatOS mark: a little hat (brim + crown + band) in `accent`, ~26px wide,
// centered on (cx, cy).
static void drawHatMark(int cx, int cy, uint16_t accent) {
  tft.fillRoundRect(cx - 15, cy + 6, 30, 5, 2, accent);      // brim
  tft.fillTriangle(cx - 10, cy + 6, cx + 10, cy + 6, cx, cy - 10, accent); // crown
  tft.fillRect(cx - 7, cy + 2, 14, 3, COL_BG);               // band cut-out
}

// Bold ~44px app glyph centered on (cx, cy), drawn in `accent`.
static void drawAppIcon(const char* id, int cx, int cy, uint16_t accent) {
  if (!strcmp(id, "counter")) {                 // dial / knob
    tft.drawCircle(cx, cy, 21, accent);
    tft.drawCircle(cx, cy, 20, accent);
    tft.fillCircle(cx, cy, 3, accent);
    tft.drawLine(cx, cy, cx + 11, cy - 13, accent);
    tft.drawLine(cx, cy, cx + 12, cy - 14, accent);
  } else if (!strcmp(id, "notes")) {            // script / document
    int w = 34, h = 42, x = cx - w / 2, y = cy - h / 2;
    tft.drawRoundRect(x, y, w, h, 4, accent);
    for (int i = 0; i < 4; i++) tft.fillRect(x + 6, y + 8 + i * 8, w - 12, 2, accent);
  } else if (!strcmp(id, "music")) {            // eighth note
    tft.fillCircle(cx - 8, cy + 12, 7, accent);
    tft.fillRect(cx - 1, cy - 16, 3, 29, accent);
    tft.fillTriangle(cx + 2, cy - 16, cx + 13, cy - 9, cx + 2, cy - 5, accent);
  } else if (!strcmp(id, "maps")) {             // location pin
    tft.fillCircle(cx, cy - 6, 15, accent);
    tft.fillTriangle(cx - 13, cy + 2, cx + 13, cy + 2, cx, cy + 22, accent);
    tft.fillCircle(cx, cy - 6, 6, COL_BG);
  } else {                                      // generic dot grid
    for (int a = 0; a < 2; a++) for (int b = 0; b < 2; b++)
      tft.fillRect(cx - 12 + a * 20, cy - 12 + b * 20, 12, 12, accent);
  }
}

void displayBootSplash() {
  tft.fillScreen(COL_BG);
  drawHatMark(80, 26, COL_PRIMARY);
  // wordmark: "Hat" white + "OS" lime, size 3, centered as a unit
  tft.setTextSize(3);
  const int cw = 18;               // size-3 char width
  int total = 5 * cw;
  int x = 80 - total / 2;
  tft.setTextColor(COL_INK);  tft.setCursor(x, 40);            tft.print("Hat");
  tft.setTextColor(COL_PRIMARY); tft.setCursor(x + 3 * cw, 40);   tft.print("OS");
  tft.fillRect(x, 64, total, 2, COL_PRIMARY);                     // accent underline
  printCentered("wearable os", 80, 70, 1, COL_DIM);
}

void displayLauncher(const char* id, const char* name, int index, int total, bool wifi) {
  uint16_t accent = appAccent(id);
  tft.fillScreen(COL_BG);

  // Header: wordmark left, wifi right, divider under.
  tft.setTextSize(1); tft.setTextColor(COL_DIM);
  tft.setCursor(4, 3); tft.print("HatOS");
  drawWifi(144, 1, wifi);
  tft.drawFastHLine(0, 14, 160, COL_FAINT);

  // Focused app: icon left, name + blurb right.
  drawAppIcon(id, 40, 46, accent);
  tft.setTextSize(2); tft.setTextColor(COL_INK);
  tft.setCursor(76, 33); tft.print(name);
  tft.setTextSize(1); tft.setTextColor(accent);
  tft.setCursor(77, 54); tft.print(appBlurb(id));

  // Rotation hint chevrons (faint).
  tft.fillTriangle(150, 42, 156, 37, 156, 47, COL_FAINT);

  // Dot pager, centered along the bottom.
  int gap = 13, startX = 80 - (total - 1) * gap / 2, dy = 72;
  for (int i = 0; i < total; i++) {
    if (i == index) tft.fillCircle(startX + i * gap, dy, 3, accent);
    else            tft.fillCircle(startX + i * gap, dy, 2, COL_FAINT);
  }
}

void displayCounter(int value) {
  tft.fillScreen(COL_BG);
  printCentered("COUNTER", 80, 8, 1, COL_DIM);
  char b[12]; snprintf(b, sizeof(b), "%d", value);
  int w = (int)strlen(b) * 24;                  // size-4 char = 24px wide
  tft.setTextSize(4); tft.setTextColor(appAccent("counter"));
  tft.setCursor(80 - w / 2, 28); tft.print(b);
  tft.fillRect(58, 68, 44, 3, appAccent("counter"));
  tft.setTextSize(1);
}

// "Now playing" card: album art LEFT (64x64 at x0..63,y8..71), text/controls
// RIGHT. Split into a static part (redrawn only on track change), the art
// bitmap, a scrolling title marquee, and a dynamic part (progress bar /
// play-pause / volume, repainted often without a fillScreen so smooth
// interpolated progress doesn't flicker).
void displayMusicArt(const uint16_t* art, bool has) {
  if (has) tft.drawRGBBitmap(0, 8, art, 64, 64);
  else     tft.fillRect(0, 8, 64, 64, COL_BG);
}

// Full non-art chrome: clears screen, draws artist + progress-bar outline
// (title itself is drawn separately by the marquee helper below).
void displayMusicStatic(const char* artist) {
  tft.fillScreen(COL_BG);
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
  tft.fillRect(64, 2, 96, 10, COL_BG);  // clear strip (right of art)
  tft.setTextColor(COL_INK);
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
        tft.drawChar(x, 3, title[i], COL_INK, COL_BG, 1);
      }
    }
  }
}

void displayMusicProgress(int pct, bool playing, int vol) {
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;

  tft.fillRect(69, 41, 88, 5, COL_BG);  // clear inside of bar
  int fillW = 88 * pct / 100;
  if (fillW > 0) tft.fillRect(69, 41, fillW, 5, 0x07E0);

  tft.fillRect(66, 50, 16, 14, COL_BG);  // clear icon area
  if (playing) {
    tft.fillTriangle(68, 52, 68, 62, 76, 57, COL_INK);
  } else {
    tft.fillRect(68, 52, 3, 10, COL_INK);
    tft.fillRect(73, 52, 3, 10, COL_INK);
  }

  tft.fillRect(106, 50, 54, 12, COL_BG);  // clear vol text
  tft.setTextColor(COL_INK);
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

// ---- Teleprompter ----------------------------------------------------------
// Word-wrap at a fixed character width (size-2 font ≈ 12 px/char) and show a
// 4-line window. Breaks on spaces where possible, honours explicit '\n', and
// hard-breaks over-long words. A slim scrollbar on the right shows position.
#define TP_CHARS   12   // characters per line at textSize 2
#define TP_VISIBLE 4    // lines shown at once
#define TP_MAXLINES 260

struct TpLine { const char* start; int len; };
static TpLine g_tp[TP_MAXLINES];

static int tpWrap(const char* text, TpLine* out, int maxLines) {
  int n = 0;
  const char* p = text;
  while (*p && n < maxLines) {
    const char* lineStart = p;
    int len = 0, lastSpace = -1;
    while (*p && *p != '\n' && len < TP_CHARS) {
      if (*p == ' ') lastSpace = len;
      p++; len++;
    }
    if (*p == '\n') {                 // explicit line break
      out[n].start = lineStart; out[n].len = len; n++;
      p++;                            // consume the newline
      continue;
    }
    if (*p == 0) {                    // last line
      out[n].start = lineStart; out[n].len = len; n++;
      break;
    }
    if (lastSpace >= 0) {             // wrap at last space on the line
      out[n].start = lineStart; out[n].len = lastSpace; n++;
      p = lineStart + lastSpace + 1;
    } else {                          // no space: hard break mid-word
      out[n].start = lineStart; out[n].len = len; n++;
    }
    while (*p == ' ') p++;            // trim spaces starting the next line
  }
  return n;
}

int teleprompterCountLines(const char* text) {
  return tpWrap(text, g_tp, TP_MAXLINES);
}

int displayTeleprompter(const char* text, int topLine) {
  int total = tpWrap(text, g_tp, TP_MAXLINES);
  if (topLine < 0) topLine = 0;
  if (total > 0 && topLine > total - 1) topLine = total - 1;

  tft.fillScreen(COL_BG);
  tft.setTextSize(2);
  for (int row = 0; row < TP_VISIBLE; row++) {
    int li = topLine + row;
    if (li >= total) break;
    tft.setTextColor(row == 0 ? 0xFFE0 : 0xC618);   // top line highlighted, rest light-grey
    char buf[TP_CHARS + 1];
    int len = g_tp[li].len; if (len > TP_CHARS) len = TP_CHARS;
    memcpy(buf, g_tp[li].start, len); buf[len] = 0;
    tft.setCursor(2, 3 + row * 19);
    tft.print(buf);
  }

  if (total > TP_VISIBLE) {             // scrollbar in the right margin
    const int trackY = 2, trackH = 76, knobH = 12;
    int maxTop = total - 1;
    int ky = trackY + (trackH - knobH) * topLine / maxTop;
    tft.fillRect(157, trackY, 2, trackH, 0x2965);
    tft.fillRect(156, ky, 4, knobH, 0x07FF);
  }
  return total;
}

// Driver's-eye turn-by-turn view: perspective road, bold turn arrow,
// distance-to-maneuver, and the upcoming street name.
void displayManeuver(const char* turn, long distanceM, const char* street) {
  tft.fillScreen(COL_BG);

  // Perspective road backdrop: trapezoid narrowing toward a vanishing point
  // near top-center, built from two filled triangles.
  const uint16_t ROAD = 0x2104;
  const uint16_t EDGE = 0x4208;
  tft.fillTriangle(52, 80, 108, 80, 88, 26, ROAD);
  tft.fillTriangle(52, 80, 88, 26, 72, 26, ROAD);
  tft.drawLine(52, 80, 72, 26, EDGE);
  tft.drawLine(108, 80, 88, 26, EDGE);

  // Dashed center-line receding upward to sell the perspective.
  tft.fillRect(77, 68, 6, 4, COL_INK);
  tft.fillRect(78, 56, 4, 3, COL_INK);
  tft.fillRect(79, 46, 3, 2, COL_INK);

  drawManeuverArrow(turn);

  // Distance-to-maneuver, top-left, bold white.
  tft.setTextColor(COL_INK);
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
