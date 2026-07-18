#pragma once
void netBegin(void (*onMessage)(const char* json));
void netLoop();
void netBroadcast(const char* json);
void hatSend(const char* json);   // unified: BLE + WS
