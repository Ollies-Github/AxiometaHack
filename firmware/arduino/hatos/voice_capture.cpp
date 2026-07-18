#include <Arduino.h>
#include <string.h>
#include <math.h>
#include "driver/adc.h"
#include "voice_capture.h"

// Analog electret mic on AX22 Port 2, output on IO0 = GPIO7 = ADC1_CH6 (verified
// on hardware). We sample the ADC with the fast low-level driver (adc1_get_raw,
// ~10us vs analogRead's jittery 30-60us) so the 16 kHz timing is consistent,
// then high-pass (kill hum/rumble) and RMS-normalize before wrapping as a WAV.
static const adc1_channel_t MIC_CH = ADC1_CHANNEL_6;   // GPIO7
static const int BTN_PIN = 16;
static const uint32_t TARGET_SR = 16000;
static const uint32_t MAX_SEC = 10;
static const size_t MAX_SAMPLES = TARGET_SR * MAX_SEC;

static uint8_t* g_wav = nullptr;
static size_t g_wavLen = 0;

void voiceBegin() {
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(MIC_CH, ADC_ATTEN_DB_11);   // ~0..3.3 V range
  g_wav = (uint8_t*)ps_malloc(44 + MAX_SAMPLES * 2);
  if (!g_wav) Serial.println("voice: PSRAM alloc FAILED");
}

static void le32(uint8_t* p, uint32_t v) { p[0]=v; p[1]=v>>8; p[2]=v>>16; p[3]=v>>24; }
static void le16(uint8_t* p, uint16_t v) { p[0]=v; p[1]=v>>8; }

size_t voiceCaptureWav(const uint8_t** outWav) {
  *outWav = nullptr;
  if (!g_wav) return 0;
  int16_t* pcm = (int16_t*)(g_wav + 44);   // holds raw 0..4095 during capture

  size_t n = 0;
  uint32_t period = 1000000UL / TARGET_SR;
  uint32_t t0 = micros();
  while (digitalRead(BTN_PIN) == HIGH && n < MAX_SAMPLES) {
    uint32_t target = t0 + (uint32_t)(n * period);
    while ((int32_t)(micros() - target) < 0) { /* spin to next sample time */ }
    pcm[n++] = (int16_t)adc1_get_raw(MIC_CH);
  }
  uint32_t elapsed = micros() - t0;
  uint32_t rate = (elapsed && n) ? (uint32_t)((uint64_t)n * 1000000ULL / elapsed) : TARGET_SR;
  if (n < 1200) { Serial.println("voice: too short"); return 0; }

  // DC bias.
  long sum = 0;
  for (size_t i = 0; i < n; i++) sum += (uint16_t)pcm[i];
  float mean = (float)sum / (float)n;

  // One-pole high-pass (~130 Hz) removes hum/rumble that muddies the speech.
  const float A = 0.95f;
  const size_t skip = (n > 3200) ? 1600 : 0;   // ignore press transient for level

  // Pass 1: filter + measure RMS of the speech body.
  float yPrev = 0, xPrev = 0; double ss = 0; size_t cnt = 0;
  for (size_t i = 0; i < n; i++) {
    float x = (float)(uint16_t)pcm[i] - mean;
    float y = A * (yPrev + x - xPrev); yPrev = y; xPrev = x;
    if (i >= skip) { ss += (double)y * y; cnt++; }
  }
  float rms = cnt ? (float)sqrt(ss / cnt) : 1.0f;
  if (rms < 1.0f) rms = 1.0f;
  float scale = 3000.0f / rms;                 // normalize toward a healthy RMS
  if (scale > 150.0f) scale = 150.0f;
  if (scale < 1.0f) scale = 1.0f;

  // Pass 2: re-run the filter, apply gain, write signed 16-bit.
  yPrev = 0; xPrev = 0;
  for (size_t i = 0; i < n; i++) {
    float x = (float)(uint16_t)pcm[i] - mean;
    float y = A * (yPrev + x - xPrev); yPrev = y; xPrev = x;
    long v = (long)(y * scale);
    if (v > 32767) v = 32767; else if (v < -32768) v = -32768;
    pcm[i] = (int16_t)v;
  }

  uint32_t dataLen = (uint32_t)n * 2;
  uint8_t* h = g_wav;
  memcpy(h, "RIFF", 4);      le32(h + 4, 36 + dataLen);  memcpy(h + 8, "WAVE", 4);
  memcpy(h + 12, "fmt ", 4); le32(h + 16, 16); le16(h + 20, 1); le16(h + 22, 1);
  le32(h + 24, rate); le32(h + 28, rate * 2); le16(h + 32, 2); le16(h + 34, 16);
  memcpy(h + 36, "data", 4); le32(h + 40, dataLen);

  g_wavLen = 44 + dataLen;
  *outWav = g_wav;
  Serial.printf("voice: %u samples @ %u Hz (rms=%.0f gain=%.1f) -> %u byte WAV\n",
                (unsigned)n, (unsigned)rate, rms, scale, (unsigned)g_wavLen);
  return g_wavLen;
}
