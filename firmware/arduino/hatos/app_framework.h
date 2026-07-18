#pragma once
#include <Arduino.h>

struct App {
  const char* id;
  const char* name;
  void (*onEnter)();
  void (*onEncoder)(int delta);
  void (*onButton)();
  void (*onMessage)(const char* json);
  void (*render)();
};

void appsBegin();
void appsEncoder(int delta);
void appsButton();
void appsMessage(const char* json);
void appsTick();
const char* appsActiveId();
void appsRenderCatalogJson(char* out, size_t n);
