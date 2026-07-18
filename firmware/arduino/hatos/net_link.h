#pragma once
#include <stddef.h>
#include <stdint.h>
void netBegin(void (*onMessage)(const char* json));
void netLoop();
void netBroadcast(const char* json);
void netBroadcastBin(const uint8_t* data, size_t len);   // e.g. captured WAV audio
void hatSend(const char* json);   // unified: BLE + WS
bool netIsConnected();            // true when joined a WiFi network (STA)
