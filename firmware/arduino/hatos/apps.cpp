#include "app_framework.h"
#include "display.h"
#include "ble_link.h"

// forward decls of the four apps
static App launcher, counterApp, textApp, musicApp;
static App* REG[] = {&launcher, &counterApp, &textApp, &musicApp};
static const int REG_N = 4;

static App* g_active = nullptr;
static int  g_launchSel = 0;
static int  g_count = 0;
static char g_text[64] = "(no text)";

// ---- small bounded JSON helpers (no JSON library) ----
static void jsonStr(const char* json, const char* key, char* out, size_t n) {
  if (n == 0) return; out[0] = 0;
  char pat[40]; snprintf(pat, sizeof(pat), "\"%s\":\"", key);
  const char* p = strstr(json, pat); if (!p) return; p += strlen(pat);
  size_t i = 0;
  while (*p && *p != '"' && i < n - 1) { if (*p == '\\' && *(p+1)) p++; out[i++] = *p++; }
  out[i] = 0;
}
static long jsonInt(const char* json, const char* key, long def) {
  char pat[40]; snprintf(pat, sizeof(pat), "\"%s\":", key);
  const char* p = strstr(json, pat); if (!p) return def; return atol(p + strlen(pat));
}
static bool jsonBoolTrue(const char* json, const char* key) {
  char pat[48]; snprintf(pat, sizeof(pat), "\"%s\":true", key);
  return strstr(json, pat) != nullptr;
}

// ---- music app state (metadata only - no audio) ----
static char g_mTitle[40]  = "—";
static char g_mArtist[40] = "";
static long g_mPos     = 0;
static long g_mDur     = 0;
static bool g_mPlaying = false;
static int  g_mVol     = 50;

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

// ---- music app (now-playing card + transport commands over BLE) ----
static void musicRender() {
  int pct = 0;
  if (g_mDur > 0) {
    pct = (int)(100 * g_mPos / g_mDur);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
  }
  displayMusic(g_mTitle, g_mArtist, pct, g_mPlaying, g_mVol);
}
static void musicEnter() { musicRender(); }
static void musicEncoder(int d) {
  g_mVol = constrain(g_mVol + d * 5, 0, 100);
  musicRender();
  char msg[80];
  snprintf(msg, sizeof(msg), "{\"type\":\"music.command\",\"payload\":{\"action\":\"volume\",\"value\":%d}}", g_mVol);
  bleSend(msg);
}
static void musicButton() {
  g_mPlaying = !g_mPlaying;
  musicRender();
  bleSend("{\"type\":\"music.command\",\"payload\":{\"action\":\"playpause\"}}");
}
static void musicMessage(const char* json) {
  if (!strstr(json, "\"music.state\"")) return;
  jsonStr(json, "title", g_mTitle, sizeof(g_mTitle));
  jsonStr(json, "artist", g_mArtist, sizeof(g_mArtist));
  g_mPos = jsonInt(json, "positionMs", g_mPos);
  g_mDur = jsonInt(json, "durationMs", g_mDur);
  g_mPlaying = jsonBoolTrue(json, "isPlaying");
  g_mVol = (int)jsonInt(json, "volume", g_mVol);
  musicRender();
}

void appsBegin() {
  launcher   = {"launcher", "Home",    nullptr, launcherEncoder, launcherButton, nullptr, launcherRender};
  counterApp = {"counter",  "Counter", nullptr, counterEncoder,  nullptr,        nullptr, counterRender};
  textApp    = {"notes",    "Notes",   nullptr, nullptr,         nullptr,        textMessage, textRender};
  musicApp   = {"music",    "Music",   musicEnter, musicEncoder, musicButton,    musicMessage, musicRender};
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
