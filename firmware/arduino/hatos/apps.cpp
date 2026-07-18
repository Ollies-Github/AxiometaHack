#include "app_framework.h"
#include "display.h"
#include "ble_link.h"
#include "net_link.h"

// forward decls of the five apps
static App launcher, counterApp, textApp, musicApp, mapsApp;
static App* REG[] = {&launcher, &counterApp, &textApp, &musicApp, &mapsApp};
static const int REG_N = 5;

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
// Position is interpolated locally between 1 Hz phone updates: g_mBasePos is
// the last reported position (ms) and g_mBaseMs is millis() at that report.
static char g_mTitle[40]  = "—";
static char g_mArtist[40] = "";
static long g_mBasePos = 0;      // position (ms) reported at last update
static uint32_t g_mBaseMs = 0;   // millis() at last update
static long g_mDur     = 0;
static bool g_mPlaying = false;
static int  g_mVol     = 50;
static uint16_t g_art[64*64];
static bool g_hasArt = false;
static int  g_titleScroll = 0;   // marquee offset in pixels

static long musicPosNow() {
  long p = g_mPlaying ? g_mBasePos + (long)(millis() - g_mBaseMs) : g_mBasePos;
  if (p < 0) p = 0;
  if (g_mDur > 0 && p > g_mDur) p = g_mDur;
  return p;
}
static int musicPct() { return (g_mDur > 0) ? (int)(100 * musicPosNow() / g_mDur) : 0; }

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
// Full redraw (fillScreen) happens only on track change or app activation;
// otherwise only the changing regions (bar fill, icon, volume) repaint, and
// position is interpolated locally so the bar animates smoothly between the
// 1 Hz music.state updates from the phone.
static void musicFull() {
  displayMusicStatic(g_mArtist);
  displayMusicArt(g_art, g_hasArt);
  displayMusicTitle(g_mTitle, g_titleScroll);
  displayMusicProgress(musicPct(), g_mPlaying, g_mVol);
}
static void musicEnter() { musicFull(); }
static void musicRender() { musicFull(); }  // used on activate
static void musicTick() {  // smooth, partial: advance marquee + interpolated progress
  g_titleScroll += 3;
  displayMusicTitle(g_mTitle, g_titleScroll);
  displayMusicProgress(musicPct(), g_mPlaying, g_mVol);
}

// Reconstruct the 64x64 RGB565 (hi-byte-first) album-art frame streamed from
// the Mac bridge over the binary WebSocket channel; redraw immediately if
// the music app is currently on screen.
void musicSetArt(const uint8_t* d, size_t len) {
  Serial.printf("art rx: %u bytes\n", (unsigned)len);
  if (len != 64*64*2) return;
  for (int i = 0; i < 64*64; i++) g_art[i] = ((uint16_t)d[2*i] << 8) | d[2*i+1];
  g_hasArt = true;
  if (g_active == &musicApp) displayMusicArt(g_art, true);
}

static void musicEncoder(int d) {
  g_mVol = constrain(g_mVol + d * 5, 0, 100);
  displayMusicProgress(musicPct(), g_mPlaying, g_mVol);  // partial, not full
  char msg[80];
  snprintf(msg, sizeof(msg), "{\"type\":\"music.command\",\"payload\":{\"action\":\"volume\",\"value\":%d}}", g_mVol);
  hatSend(msg);
}
static void musicButton() {  // keep existing playpause behavior
  g_mPlaying = !g_mPlaying; g_mBasePos = musicPosNow(); g_mBaseMs = millis();
  displayMusicProgress(musicPct(), g_mPlaying, g_mVol);
  hatSend("{\"type\":\"music.command\",\"payload\":{\"action\":\"playpause\"}}");
}
static void musicDouble() { hatSend("{\"type\":\"music.command\",\"payload\":{\"action\":\"next\"}}"); }
static void musicMessage(const char* json) {
  if (!strstr(json, "\"music.state\"")) return;
  char title[40], artist[40];
  jsonStr(json, "title", title, sizeof(title));
  jsonStr(json, "artist", artist, sizeof(artist));
  bool trackChanged = (strcmp(title, g_mTitle) != 0) || (strcmp(artist, g_mArtist) != 0);
  strncpy(g_mTitle, title, sizeof(g_mTitle) - 1); g_mTitle[sizeof(g_mTitle) - 1] = 0;
  strncpy(g_mArtist, artist, sizeof(g_mArtist) - 1); g_mArtist[sizeof(g_mArtist) - 1] = 0;
  g_mDur = jsonInt(json, "durationMs", 0);
  g_mPlaying = jsonBoolTrue(json, "isPlaying");
  g_mVol = (int)jsonInt(json, "volume", g_mVol);
  g_mBasePos = jsonInt(json, "positionMs", 0);
  g_mBaseMs = millis();
  if (trackChanged) {
    g_titleScroll = 0;
    displayMusicStatic(g_mArtist);
    displayMusicArt(g_art, g_hasArt);
    displayMusicTitle(g_mTitle, g_titleScroll);
  } else {
    displayMusicTitle(g_mTitle, g_titleScroll);
  }
  displayMusicProgress(musicPct(), g_mPlaying, g_mVol);
}

// ---- maps app (driver's-eye turn-by-turn view, pushed from the phone) ----
static char g_turn[16]   = "straight";
static long g_dist       = 0;
static char g_street[40] = "";

static void mapsRender() { displayManeuver(g_turn, g_dist, g_street); }
static void mapsEnter()  { mapsRender(); }
static void mapsMessage(const char* json) {
  if (!strstr(json, "\"maps.maneuver\"")) return;
  jsonStr(json, "turn", g_turn, sizeof(g_turn));
  g_dist = jsonInt(json, "distanceM", g_dist);
  jsonStr(json, "street", g_street, sizeof(g_street));
  mapsRender();
}

void appsBegin() {
  launcher   = {"launcher", "Home",    nullptr, launcherEncoder, launcherButton, nullptr, launcherRender, nullptr, nullptr};
  counterApp = {"counter",  "Counter", nullptr, counterEncoder,  nullptr,        nullptr, counterRender, nullptr, nullptr};
  textApp    = {"notes",    "Notes",   nullptr, nullptr,         nullptr,        textMessage, textRender, nullptr, nullptr};
  musicApp   = {"music",    "Music",   musicEnter, musicEncoder, musicButton,    musicMessage, musicRender, musicDouble, musicTick};
  mapsApp    = {"maps",     "Maps",    mapsEnter,  nullptr,      nullptr,        mapsMessage,  mapsRender, nullptr, nullptr};
  activate(&launcher);
}

// single press goes to the app's button handler; long-press returns home (see appsGoHome)
void appsEncoder(int d) { if (g_active && g_active->onEncoder) g_active->onEncoder(d); }
void appsButton()       { if (g_active && g_active->onButton)  g_active->onButton(); }
void appsGoHome()       { activate(&launcher); }
void appsDoubleButton() { if (g_active && g_active->onDoubleButton) g_active->onDoubleButton(); }
void appsMessage(const char* json) {
  if (strstr(json, "\"app.setActive\"")) {
    const char* p = strstr(json, "\"id\":\"");
    if (p) { char id[24]; size_t i = 0; p += 6; while (*p && *p != '"' && i < 23) id[i++] = *p++; id[i] = 0;
             App* a = findById(id); if (a) activate(a); }
    return;
  }
  if (g_active && g_active->onMessage) g_active->onMessage(json);
}
void appsTick() { if (g_active && g_active->onTick) g_active->onTick(); }
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
