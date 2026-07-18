#pragma once
#include <stdint.h>
#include <stddef.h>

// Onboard analog-mic capture (Port 2 IO0 = GPIO7). Records 16-bit PCM into a
// PSRAM buffer while the AI button is held and packages it as a WAV.
void voiceBegin();

// Blocking: sample the mic while the button (GPIO16) is held, up to a cap, then
// build a mono 16-bit WAV. Returns the WAV length and points *outWav at it
// (buffer owned here, valid until the next call). 0 if nothing captured.
size_t voiceCaptureWav(const uint8_t** outWav);
