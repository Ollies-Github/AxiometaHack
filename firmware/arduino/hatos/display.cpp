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
