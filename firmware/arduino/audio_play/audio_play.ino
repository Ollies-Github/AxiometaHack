// Axiometa Genesis Mini — stream an embedded PCM clip to the MAX98357A (Port 2)
// -----------------------------------------------------------------------------
// Plays clip.raw (mono, 16 kHz, 16-bit LE) embedded via audio_data.S over I2S.
//   Amp (Port 2): DIN=GPIO7, BCLK=GPIO6, LRCLK=GPIO5
//   Encoder (Port 1): turn = volume, press = pause/resume (long feel: restarts)
//   TFT (Port 4): now-playing + progress bar
//
// Build: PartitionScheme=huge_app (room for the embedded audio)
// FQBN: esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=4M,PSRAM=enabled,PartitionScheme=huge_app
// -----------------------------------------------------------------------------

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <driver/i2s.h>

extern const uint8_t audio_pcm[];
extern const uint8_t audio_pcm_end[];

// ---- Display ----------------------------------------------------------------
constexpr int TFT_MOSI = 12, TFT_SCLK = 14, TFT_CS = 1, TFT_DC = 18, TFT_RST = 17;
SPIClass tftSPI(FSPI);
Adafruit_ST7735 tft(&tftSPI, TFT_CS, TFT_DC, TFT_RST);

// ---- I2S amp (Port 2) -------------------------------------------------------
constexpr int I2S_DOUT = 7, I2S_BCLK = 6, I2S_LRC = 5;
constexpr int SR = 16000;

// ---- Encoder (Port 1) -------------------------------------------------------
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
int  volume = 70;                 // 0..100
bool paused = false, lastBtnRaw = false;
uint32_t lastBtnMs = 0;

const int16_t *samples;
size_t nSamp, playIdx = 0;
int16_t stereo[512];
int lastPct = -1, lastVol = -1; bool lastPaused = true;

void drawStatic() {
  tft.fillScreen(0x0002);
  tft.setTextSize(2); tft.setTextColor(0x07E0); tft.setCursor(6, 4);  tft.print("NOW");
  tft.setTextColor(0xFD20); tft.setCursor(52, 4); tft.print("PLAYING");
  tft.setTextSize(1); tft.setTextColor(0x07FF); tft.setCursor(6, 26); tft.print("Nettspend");
  tft.setTextColor(0xFFFF); tft.setCursor(6, 36); tft.print("Nothing Like UUU");
  tft.drawRect(4, 52, 152, 10, 0x8410);
  tft.setTextColor(0x8410); tft.setCursor(6, 68); tft.print("turn=vol press=pause");
}
void drawStatus() {
  int pct = (int)(100.0 * playIdx / nSamp);
  if (pct != lastPct) {
    lastPct = pct;
    tft.fillRect(5, 53, 150 * pct / 100, 8, paused ? 0xFD20 : 0x07E0);
  }
  if (volume != lastVol || paused != lastPaused) {
    lastVol = volume; lastPaused = paused;
    tft.fillRect(90, 26, 70, 9, 0x0002);
    tft.setTextSize(1); tft.setTextColor(paused ? 0xFD20 : 0xFFFF); tft.setCursor(90, 26);
    tft.printf(paused ? "PAUSE" : "vol %d", volume);
  }
}

void pollControls() {
  noInterrupts(); int32_t raw = encRaw; interrupts();
  long p = raw / 4;
  if (p != lastPos) { volume = constrain(volume + (int)(p - lastPos) * 5, 0, 100); lastPos = p; }
  bool rb = (digitalRead(PIN_BTN) == HIGH);
  if (rb != lastBtnRaw && millis() - lastBtnMs > 25) {
    lastBtnRaw = rb; lastBtnMs = millis();
    if (rb) { paused = !paused; if (paused) i2s_zero_dma_buffer(I2S_NUM_0); }
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  samples = (const int16_t *)audio_pcm;
  nSamp = (audio_pcm_end - audio_pcm) / 2;
  Serial.printf("\n== PCM player: %u samples (%.1fs) ==\n", (unsigned)nSamp, (float)nSamp / SR);

  pinMode(PIN_A, INPUT_PULLUP); pinMode(PIN_B, INPUT_PULLUP); pinMode(PIN_BTN, INPUT);
  encPrev = (uint8_t)((digitalRead(PIN_A) << 1) | digitalRead(PIN_B));
  attachInterrupt(digitalPinToInterrupt(PIN_A), encISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_B), encISR, CHANGE);

  tftSPI.begin(TFT_SCLK, -1, TFT_MOSI);
  tft.initR(INITR_MINI160x80); tft.setRotation(3); tft.invertDisplay(true);
  drawStatic();

  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = SR;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.dma_buf_count = 8;
  cfg.dma_buf_len = 256;
  cfg.tx_desc_auto_clear = true;
  i2s_pin_config_t pins = {};
  pins.bck_io_num = I2S_BCLK; pins.ws_io_num = I2S_LRC;
  pins.data_out_num = I2S_DOUT; pins.data_in_num = I2S_PIN_NO_CHANGE;
  i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pins);
}

void loop() {
  pollControls();
  drawStatus();

  if (paused) { delay(20); return; }

  // build one stereo chunk (duplicate mono sample to L+R, apply volume)
  int j = 0;
  int vol256 = (volume * 256) / 100;
  while (j < 512 && playIdx < nSamp) {
    int32_t s = ((int32_t)samples[playIdx++] * vol256) >> 8;
    stereo[j++] = (int16_t)s;   // L
    stereo[j++] = (int16_t)s;   // R
  }
  size_t bw;
  i2s_write(I2S_NUM_0, stereo, j * 2, &bw, portMAX_DELAY);

  if (playIdx >= nSamp) { playIdx = 0; lastPct = -1; }   // loop
}
