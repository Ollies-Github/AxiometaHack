#include "app_framework.h"
#include "display.h"

// forward decls of the three apps
static App launcher, counterApp, textApp;
static App* REG[] = {&launcher, &counterApp, &textApp};
static const int REG_N = 3;

static App* g_active = nullptr;
static int  g_launchSel = 0;
static int  g_count = 0;
static char g_text[64] = "(no text)";

static void activate(App* a) { g_active = a; if (a->onEnter) a->onEnter(); if (a->render) a->render(); }
static App* findById(const char* id) {
  for (int i = 0; i < REG_N; i++) if (strcmp(REG[i]->id, id) == 0) return REG[i];
  return nullptr;
}

// ---- launcher ----
static void launcherRender() {
  const char* l0 = "Home";
  char l1[24]; snprintf(l1, sizeof(l1), "> %s", REG[g_launchSel]->name);
  const char* lines[] = {l0, l1};
  displayLines(lines, 2);
}
static void launcherEncoder(int d) {
  g_launchSel = (g_launchSel + d) % REG_N;
  if (g_launchSel < 0) g_launchSel += REG_N;
  launcherRender();
}
static void launcherButton() { activate(REG[g_launchSel]); }

// ---- counter app ----
static void counterRender() {
  char l1[24]; snprintf(l1, sizeof(l1), "%d", g_count);
  const char* lines[] = {"Counter", l1};
  displayLines(lines, 2);
}
static void counterEncoder(int d) { g_count += d; counterRender(); }

// ---- text app (shows text pushed from phone) ----
static void textRender() { const char* lines[] = {"Notes", g_text}; displayLines(lines, 2); }
static void textMessage(const char* json) {
  const char* p = strstr(json, "\"lines\":[\"");
  if (p) { p += 10; size_t i = 0; while (*p && *p != '"' && i < sizeof(g_text) - 1) g_text[i++] = *p++; g_text[i] = 0; }
  textRender();
}

void appsBegin() {
  launcher   = {"launcher", "Home",    nullptr, launcherEncoder, launcherButton, nullptr, launcherRender};
  counterApp = {"counter",  "Counter", nullptr, counterEncoder,  nullptr,        nullptr, counterRender};
  textApp    = {"notes",    "Notes",   nullptr, nullptr,         nullptr,        textMessage, textRender};
  activate(&launcher);
}

// long-press returns home; short press goes to the app's button handler
void appsEncoder(int d) { if (g_active && g_active->onEncoder) g_active->onEncoder(d); }
void appsButton()       { if (g_active && g_active->onButton)  g_active->onButton(); }
void appsMessage(const char* json) {
  if (strstr(json, "\"app.setActive\"")) {
    const char* p = strstr(json, "\"id\":\"");
    if (p) { char id[24]; size_t i = 0; p += 6; while (*p && *p != '"' && i < 23) id[i++] = *p++; id[i] = 0;
             App* a = findById(id); if (a) activate(a); }
    return;
  }
  if (g_active && g_active->onMessage) g_active->onMessage(json);
}
void appsTick() { if (g_active && g_active->render) g_active->render(); }
const char* appsActiveId() { return g_active ? g_active->id : "launcher"; }
void appsRenderCatalogJson(char* out, size_t n) {
  if (n == 0) return;
  size_t k = 0;
  int w = snprintf(out + k, n - k, "[");
  if (w < 0) { out[n ? n - 1 : 0] = 0; return; }
  if ((size_t)w >= n - k) { out[n - 1] = 0; return; }
  k += (size_t)w;
  for (int i = 0; i < REG_N; i++) {
    w = snprintf(out + k, n - k, "%s{\"id\":\"%s\",\"name\":\"%s\"}", i ? "," : "", REG[i]->id, REG[i]->name);
    if (w < 0) { out[n ? n - 1 : 0] = 0; return; }
    if ((size_t)w >= n - k) { out[n - 1] = 0; return; }
    k += (size_t)w;
  }
  w = snprintf(out + k, n - k, "]");
  if (w < 0) { out[n ? n - 1 : 0] = 0; return; }
  if ((size_t)w >= n - k) { out[n - 1] = 0; return; }
  k += (size_t)w;
}
