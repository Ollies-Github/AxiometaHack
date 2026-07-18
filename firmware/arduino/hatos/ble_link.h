#pragma once
void bleBegin(void (*onMessage)(const char* json));
void bleSend(const char* json);
void bleFlush();
bool bleConnected();
bool bleSubscribed();
