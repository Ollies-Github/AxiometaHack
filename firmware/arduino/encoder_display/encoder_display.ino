// Axiometa Genesis Mini — Encoder (P1) + IPS TFT (P4) + Servo (P3) + Buzzer (P2)
// -----------------------------------------------------------------------------
// VERIFIED wiring:
//   Encoder (Port 1, EC11): A/CLK=GPIO3, B/DT=GPIO2, SW=GPIO4 (pressed=HIGH)
//   Display (Port 4, ST7735S 160x80 SPI): MOSI=12, SCK=14, CS=1, DC=18, RST=17
//   Buzzer  (Port 2, MLT-8530): PWM on IO1 = GPIO6
//   Servo   (Port 3, AX22-0031 adapter): Servo 1 = IO0 = GPIO9  (J3 signal)
//
// Behaviour:
//   * Turn knob  -> set servo angle 0..180 (CW = up), tick on each step
//   * Press knob -> toggle AUTO-SWEEP (servo sweeps 0<->180 on its own)
//
// FQBN: esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=4M,PSRAM=enabled
// -----------------------------------------------------------------------------

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <math.h>

// ---- Display ----------------------------------------------------------------
constexpr int TFT_MOSI = 12, TFT_SCLK = 14, TFT_CS = 1, TFT_DC = 18, TFT_RST = 17;
SPIClass tftSPI(FSPI);
Adafruit_ST7735 tft(&tftSPI, TFT_CS, TFT_DC, TFT_RST);
constexpr int W = 160, H = 80;

// ---- Servo (Port 3) ---------------------------------------------------------
// Drive ALL three servo-adapter slots so whichever slot the servo is in moves.
//   slot pins = Port3 IO0/IO1/IO2 = GPIO9 / GPIO16 / GPIO15
const int SERVO_PINS[3] = {9, 16, 15};
const int SERVO_CHS[3]  = {2, 3, 4};                 // LEDC channels (50 Hz)
void servoWrite(int ang) {
  ang = constrain(ang, 0, 180);
  int us = 500 + (2000 * ang) / 180;                 // 500..2500 us pulse
  uint32_t duty = (uint32_t)us * 65535UL / 20000UL;  // 20 ms period @ 16-bit
  for (int i = 0; i < 3; i++) ledcWrite(SERVO_CHS[i], duty);
}

// ---- Buzzer (Port 2, GPIO6) -------------------------------------------------
constexpr int BUZZER = 6, BUZ_CH = 0;
uint32_t beepOff = 0;
void beep(int f, int ms) { ledcWriteTone(BUZ_CH, f); beepOff = millis() + ms; }

// ---- Encoder ----------------------------------------------------------------
constexpr int PIN_A = 3, PIN_B = 2, PIN_BTN = 4;
volatile int32_t encRaw = 0;
volatile uint8_t encPrev = 3;
const int8_t QTAB[16] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
void IRAM_ATTR encISR() {
  uint8_t s = (uint8_t)((digitalRead(PIN_A) << 1) | digitalRead(PIN_B));
  encRaw += QTAB[((encPrev << 2) | s) & 0x0F];
  encPrev = s;
}
long lastPos = 0;

// ---- State ------------------------------------------------------------------
int  angle = 90, shownAngle = -1;
bool sweep = false, sweepUp = true;
uint32_t lastSweep = 0;
bool btnPressed = false, lastBtnRaw = false;
uint32_t lastBtnMs = 0;

// gauge geometry (servo-horn style semicircle)
const int GX = 80, GY = 74, GR = 52;
int armPx = GX, armPy = GY;

void armXY(int a, int &x, int &y) {
  float sa = (180 - a) * DEG_TO_RAD;                 // 0deg->left, 180deg->right
  x = GX + (int)((GR - 4) * cosf(sa));
  y = GY - (int)((GR - 4) * sinf(sa));
}

void drawChrome() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(0x07FF); tft.setCursor(2, 2);  tft.print("SERVO");
  tft.setTextColor(0x8410); tft.setCursor(44, 2); tft.print("Port3 / Servo1");
  // gauge arc + ticks
  for (int a = 0; a <= 180; a += 2) {
    float sa = (180 - a) * DEG_TO_RAD;
    tft.drawPixel(GX + (int)(GR * cosf(sa)), GY - (int)(GR * sinf(sa)), 0x4208);
  }
  for (int a = 0; a <= 180; a += 45) {
    float sa = (180 - a) * DEG_TO_RAD;
    tft.drawLine(GX + (int)((GR-6)*cosf(sa)), GY - (int)((GR-6)*sinf(sa)),
                 GX + (int)(GR*cosf(sa)),     GY - (int)(GR*sinf(sa)), 0x8410);
  }
  tft.fillCircle(GX, GY, 3, 0x8410);
}

void drawAngle() {
  // erase old arm, draw new
  tft.drawLine(GX, GY, armPx, armPy, ST77XX_BLACK);
  tft.fillCircle(armPx, armPy, 2, ST77XX_BLACK);
  int x, y; armXY(angle, x, y);
  uint16_t col = sweep ? 0xFD20 : 0x07E0;
  tft.drawLine(GX, GY, x, y, col);
  tft.fillCircle(x, y, 3, ST77XX_WHITE);
  tft.fillCircle(GX, GY, 3, col);
  armPx = x; armPy = y;

  tft.fillRect(0, 14, 90, 26, ST77XX_BLACK);
  tft.setTextSize(3); tft.setTextColor(col); tft.setCursor(4, 15);
  tft.printf("%d", angle);
  tft.setTextSize(1); tft.setCursor(4, 40);
  tft.setTextColor(0xFFFF); tft.print("deg");
  tft.fillRect(92, 40, 68, 9, ST77XX_BLACK);
  tft.setTextColor(sweep ? 0xFD20 : 0x8410); tft.setCursor(92, 40);
  tft.print(sweep ? "SWEEP" : "press=sweep");
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n== Mini: encoder -> servo (Port3/Servo1, GPIO9) ==");

  pinMode(PIN_A, INPUT_PULLUP);
  pinMode(PIN_B, INPUT_PULLUP);
  pinMode(PIN_BTN, INPUT);
  encPrev = (uint8_t)((digitalRead(PIN_A) << 1) | digitalRead(PIN_B));
  attachInterrupt(digitalPinToInterrupt(PIN_A), encISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_B), encISR, CHANGE);

  ledcSetup(BUZ_CH, 2700, 10);  ledcAttachPin(BUZZER, BUZ_CH);  ledcWriteTone(BUZ_CH, 0);
  for (int i = 0; i < 3; i++) { ledcSetup(SERVO_CHS[i], 50, 16); ledcAttachPin(SERVO_PINS[i], SERVO_CHS[i]); }
  servoWrite(angle);

  tftSPI.begin(TFT_SCLK, -1, TFT_MOSI);   // <-- was missing: init display SPI
  tft.initR(INITR_MINI160x80);
  tft.setRotation(3);
  tft.invertDisplay(true);

  drawChrome();
  drawAngle();
}

void loop() {
  // button -> toggle sweep
  bool raw_b = (digitalRead(PIN_BTN) == HIGH);
  if (raw_b != lastBtnRaw && (millis() - lastBtnMs) > 25) {
    lastBtnRaw = raw_b; lastBtnMs = millis();
    if (raw_b != btnPressed) {
      btnPressed = raw_b;
      if (btnPressed) { sweep = !sweep; beep(sweep ? 2093 : 1568, 80);
                        Serial.printf("sweep %s\n", sweep ? "ON" : "OFF"); }
    }
  }

  // encoder -> angle (manual mode)
  noInterrupts(); int32_t raw = encRaw; interrupts();
  long p = raw / 4;
  if (p != lastPos) {
    int delta = (int)(p - lastPos);
    lastPos = p;
    if (!sweep) { angle = constrain(angle + delta * 5, 0, 180); beep(3136, 18); }
  }

  // auto sweep
  if (sweep && millis() - lastSweep > 15) {
    lastSweep = millis();
    angle += sweepUp ? 2 : -2;
    if (angle >= 180) { angle = 180; sweepUp = false; }
    if (angle <= 0)   { angle = 0;   sweepUp = true;  }
  }

  if (beepOff && millis() >= beepOff) { ledcWriteTone(BUZ_CH, 0); beepOff = 0; }

  if (angle != shownAngle) {
    shownAngle = angle;
    servoWrite(angle);
    drawAngle();
    Serial.printf("angle=%d\n", angle);
  }
  delay(2);
}
