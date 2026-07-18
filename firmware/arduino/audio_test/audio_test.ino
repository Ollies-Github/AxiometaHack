// Axiometa Genesis Mini — MAX98357A I2S amp (Port 2) audio test
// -----------------------------------------------------------------------------
// Amp module AX22-0053 (MAX98357A), on PORT 2:
//   DIN   = IO0 = GPIO7
//   BCLK  = IO1 = GPIO6
//   LRCLK = IO2 = GPIO5
//   (VDD from port 5V; SD_MODE/GAIN set by on-board jumpers)
//
// Plays a looping test melody (real sine tones over I2S).
//   Turn encoder (Port 1) = volume,  Press = mute/unmute.
//   TFT (Port 4) shows status + VU meter.
//
// FQBN: esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=4M,PSRAM=enabled
// -----------------------------------------------------------------------------

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <driver/i2s.h>
#include <math.h>

// ---- Display (Port 4) -------------------------------------------------------
constexpr int TFT_MOSI = 12, TFT_SCLK = 14, TFT_CS = 1, TFT_DC = 18, TFT_RST = 17;
SPIClass tftSPI(FSPI);
Adafruit_ST7735 tft(&tftSPI, TFT_CS, TFT_DC, TFT_RST);

// ---- I2S amp (Port 2) -------------------------------------------------------
constexpr int I2S_DOUT = 7;   // DIN  = IO0
constexpr int I2S_BCLK = 6;   // BCLK = IO1
constexpr int I2S_LRC  = 5;   // LRCLK= IO2
constexpr int SR = 22050;

// ---- Encoder (Port 1) for volume/mute --------------------------------------
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
int  volume = 55;             // 0..100
bool muted = false, lastBtnRaw = false;
uint32_t lastBtnMs = 0;

// ---- test melody: C major scale up + arpeggio down (unambiguously correct) --
struct N { float f; int ms; };
const N MEL[] = {
  {523,200},{587,200},{659,200},{698,200},{784,200},{880,200},{988,200},{1047,320},
  {1047,160},{784,160},{659,160},{523,360},{0,240},
};
const int NMEL = sizeof(MEL) / sizeof(MEL[0]);
int melIdx = 0;
float curFreq = 0;

// ---- TFT ui -----------------------------------------------------------------
void drawStatic() {
  tft.fillScreen(0x0008);
  tft.setTextSize(2); tft.setTextColor(0x07E0); tft.setCursor(6, 4);  tft.print("AUDIO OK");
  tft.setTextSize(1); tft.setTextColor(0x07FF); tft.setCursor(6, 24); tft.print("MAX98357A  Port2 I2S");
  tft.setTextColor(0x8410); tft.setCursor(6, 68); tft.print("turn=vol  press=mute");
}
int lastVol = -1; bool lastMute = !muted; float lastDrawnF = -1;
void drawStatus() {
  if (volume != lastVol || muted != lastMute) {
    lastVol = volume; lastMute = muted;
    tft.fillRect(0, 36, 160, 12, 0x0008);
    tft.setTextSize(1); tft.setCursor(6, 38);
    tft.setTextColor(muted ? 0xF800 : 0xFFFF);
    tft.printf(muted ? "VOL %d  [MUTED]" : "VOL %d", volume);
  }
  // VU bar from current note
  tft.fillRect(0, 52, 160, 12, 0x0008);
  int w = muted ? 0 : (int)(curFreq > 0 ? map(constrain((int)curFreq, 400, 1100), 400, 1100, 20, 152) : 4);
  for (int x = 0; x < w; x += 6) {
    uint16_t c = (x > 110) ? 0xF800 : (x > 70) ? 0xFD20 : 0x07E0;
    tft.fillRect(4 + x, 52, 4, 10, c);
  }
}

// ---- audio ------------------------------------------------------------------
int16_t buf[256];
void playTone(float freq, int ms) {
  curFreq = freq;
  int total = (int)((long)SR * ms / 1000);
  int fade = SR * 6 / 1000;                    // 6 ms attack/release (no clicks)
  double phase = 0, dp = 2.0 * M_PI * freq / SR;
  float vol = muted ? 0.0f : volume / 100.0f;
  int idx = 0;
  for (int s = 0; s < total; s++) {
    float env = 1.0f;
    if (s < fade) env = (float)s / fade;
    else if (s > total - fade) env = (float)(total - s) / fade;
    int16_t v = (freq <= 0) ? 0 : (int16_t)(sinf((float)phase) * 30000.0f * vol * env);
    phase += dp; if (phase > 2 * M_PI) phase -= 2 * M_PI;
    buf[idx++] = v; buf[idx++] = v;            // L, R
    if (idx >= 256) { size_t bw; i2s_write(I2S_NUM_0, buf, idx * 2, &bw, portMAX_DELAY); idx = 0; }
  }
  if (idx) { size_t bw; i2s_write(I2S_NUM_0, buf, idx * 2, &bw, portMAX_DELAY); }
}

void pollControls() {
  noInterrupts(); int32_t raw = encRaw; interrupts();
  long p = raw / 4;
  if (p != lastPos) { volume = constrain(volume + (int)(p - lastPos) * 5, 0, 100); lastPos = p; }
  bool rb = (digitalRead(PIN_BTN) == HIGH);
  if (rb != lastBtnRaw && millis() - lastBtnMs > 25) {
    lastBtnRaw = rb; lastBtnMs = millis();
    if (rb) { muted = !muted; Serial.printf("mute %d\n", muted); }
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n== MAX98357A I2S audio test (Port 2) ==");

  pinMode(PIN_A, INPUT_PULLUP); pinMode(PIN_B, INPUT_PULLUP); pinMode(PIN_BTN, INPUT);
  encPrev = (uint8_t)((digitalRead(PIN_A) << 1) | digitalRead(PIN_B));
  attachInterrupt(digitalPinToInterrupt(PIN_A), encISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_B), encISR, CHANGE);

  // display
  tftSPI.begin(TFT_SCLK, -1, TFT_MOSI);
  tft.initR(INITR_MINI160x80); tft.setRotation(3); tft.invertDisplay(true);
  drawStatic();

  // I2S
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = SR;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;     // stereo frames, same data both
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = 0;
  cfg.dma_buf_count = 8;
  cfg.dma_buf_len = 256;
  cfg.use_apll = false;
  cfg.tx_desc_auto_clear = true;
  i2s_pin_config_t pins = {};
  pins.bck_io_num = I2S_BCLK;
  pins.ws_io_num = I2S_LRC;
  pins.data_out_num = I2S_DOUT;
  pins.data_in_num = I2S_PIN_NO_CHANGE;
  esp_err_t e1 = i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  esp_err_t e2 = i2s_set_pin(I2S_NUM_0, &pins);
  Serial.printf("i2s install=%d set_pin=%d\n", e1, e2);

  // startup chime so you instantly know it's alive
  playTone(660, 120); playTone(990, 200);
}

void loop() {
  const N &n = MEL[melIdx];
  pollControls();
  drawStatus();
  Serial.printf("note %.0f Hz  vol=%d mute=%d\n", n.f, volume, muted);
  playTone(n.f, n.ms);
  melIdx = (melIdx + 1) % NMEL;
}
