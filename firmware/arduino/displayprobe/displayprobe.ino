// Display pin-order brute force.
// Shared SPI: MOSI=12, SCK=14. Port-4 control pins are {1,17,18} but their
// CS/DC/RST order is unknown -> try all 6 permutations, 4s each, labelled.
// Also polls the encoder (GPIO3/2/4) and shows a live count, so the correct
// combo demonstrates the encoder too. Everything is echoed to serial.

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

constexpr int PIN_MOSI = 12, PIN_SCLK = 14;
SPIClass tftSPI(FSPI);

// {CS, DC, RST}
int combo[6][3] = {
  {17, 18, 1}, {17, 1, 18},
  {18, 17, 1}, {18, 1, 17},
  {1, 17, 18}, {1, 18, 17},
};
uint16_t bg[6] = {0x001F, 0x07E0, 0xF800, 0xFFE0, 0x07FF, 0xF81F};

Adafruit_ST7735 *tft = nullptr;
int idx = -1;
uint32_t tSwitch = 0;

// encoder (polled)
constexpr int PIN_A = 3, PIN_B = 2, PIN_BTN = 4;
long encRaw = 0, lastDet = LONG_MIN;
uint8_t encPrev = 3;
const int8_t QTAB[16] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};

void switchCombo() {
  if (tft) { delete tft; tft = nullptr; }
  int cs = combo[idx][0], dc = combo[idx][1], rst = combo[idx][2];
  Serial.printf("\n[#%d] CS=%d DC=%d RST=%d  <-- if THIS frame is clean, tell me #%d\n",
                idx, cs, dc, rst, idx);
  tft = new Adafruit_ST7735(&tftSPI, cs, dc, rst);
  tft->initR(INITR_MINI160x80);
  tft->setRotation(3);
  tft->invertDisplay(true);
  tft->fillScreen(bg[idx]);
  tft->setTextColor(ST77XX_BLACK);
  tft->setTextSize(3);
  tft->setCursor(6, 6);
  tft->printf("#%d", idx);
  tft->setTextSize(1);
  tft->setCursor(6, 36);
  tft->printf("CS%d DC%d RS%d", cs, dc, rst);
  lastDet = LONG_MIN;
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n== DISPLAY PROBE: 6 CS/DC/RST combos ==");
  pinMode(PIN_A, INPUT_PULLUP);
  pinMode(PIN_B, INPUT_PULLUP);
  pinMode(PIN_BTN, INPUT);
  encPrev = (uint8_t)((digitalRead(PIN_A) << 1) | digitalRead(PIN_B));
  tftSPI.begin(PIN_SCLK, -1, PIN_MOSI);
}

void loop() {
  // poll encoder
  uint8_t s = (uint8_t)((digitalRead(PIN_A) << 1) | digitalRead(PIN_B));
  encRaw += QTAB[((encPrev << 2) | s) & 0x0F];
  encPrev = s;
  long det = encRaw / 4;

  if (idx < 0 || millis() - tSwitch > 4000) {
    idx = (idx + 1) % 6;
    tSwitch = millis();
    switchCombo();
  }

  if (tft && det != lastDet) {
    lastDet = det;
    tft->fillRect(0, 52, 160, 28, bg[idx]);
    tft->setTextColor(ST77XX_WHITE);
    tft->setTextSize(3);
    tft->setCursor(6, 52);
    tft->printf("ENC %ld", det);
    Serial.printf("   enc=%ld btn=%d\n", det, digitalRead(PIN_BTN));
  }
  delay(2);
}
