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

// ---- launcher (home carousel) ----
// The carousel lists the launchable apps only — REG[1..REG_N-1] — since REG[0]
// is the launcher itself and "open Home from Home" makes no sense.
static const int LAUNCH_N = REG_N - 1;
static void launcherRender() {
  App* a = REG[g_launchSel + 1];
  displayLauncher(a->id, a->name, g_launchSel, LAUNCH_N, netIsConnected());
}
static void launcherEncoder(int d) {
  g_launchSel = (g_launchSel + d) % LAUNCH_N;
  if (g_launchSel < 0) g_launchSel += LAUNCH_N;
  launcherRender();
}
static void launcherButton() { activate(REG[g_launchSel + 1]); }
static void launcherDouble() {   // double-press on Home flips the theme
  displaySetTheme(!displayThemeLight());
  launcherRender();
}

// ---- counter app ----
static void counterRender() { displayCounter(g_count); }
static void counterEncoder(int d) { g_count += d; counterRender(); }

// ---- notes / teleprompter app ----
// Holds a full script pushed from the phone; the encoder scrolls it one wrapped
// line per detent. The script arrives as
// {"type":"notes.script","payload":{"text":"..."}} with JSON-escaped newlines.
static char g_script[2048] = "Push a script from the phone, then turn the knob to scroll.";
static int  g_topLine   = 0;
static int  g_lineCount = 1;

// Copy the "text" field out of `json` into g_script, unescaping JSON string
// escapes (\n -> newline, \t -> space, \" \\ \/ literal, \r dropped, \uXXXX -> ?).
static void parseScript(const char* json) {
  const char* p = strstr(json, "\"text\":\"");
  if (!p) return;
  p += 8;
  size_t i = 0;
  while (*p && *p != '"' && i < sizeof(g_script) - 1) {
    if (*p == '\\' && *(p + 1)) {
      p++;
      switch (*p) {
        case 'n': g_script[i++] = '\n'; break;
        case 't': g_script[i++] = ' ';  break;
        case 'r': break;
        case 'u': for (int k = 0; k < 4 && *(p + 1); k++) p++; g_script[i++] = '?'; break;
        default:  g_script[i++] = *p;    break;   // covers \" \\ \/ and any other
      }
      p++;
    } else {
      g_script[i++] = *p++;
    }
  }
  g_script[i] = 0;
}

static void notesRender() { g_lineCount = displayTeleprompter(g_script, g_topLine); }
static void notesEnter()  { g_topLine = 0; notesRender(); }
static void notesEncoder(int d) {
  int maxTop = (g_lineCount > 0) ? g_lineCount - 1 : 0;
  g_topLine += d;
  if (g_topLine < 0) g_topLine = 0;
  if (g_topLine > maxTop) g_topLine = maxTop;
  notesRender();
}
static void notesMessage(const char* json) {
  if (!strstr(json, "\"notes.script\"")) return;
  parseScript(json);
  g_topLine = 0;
  g_lineCount = teleprompterCountLines(g_script);
  if (g_active == &textApp) notesRender();
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
  // Update state always, but only draw when Music is on screen — otherwise the
  // bridge's 1 Hz updates would stomp whatever app the user switched to.
  if (g_active != &musicApp) return;
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
  if (g_active == &mapsApp) mapsRender();   // only draw when Maps is on screen
}

void appsBegin() {
  launcher   = {"launcher", "Home",    nullptr, launcherEncoder, launcherButton, nullptr, launcherRender, launcherDouble, nullptr};
  counterApp = {"counter",  "Counter", nullptr, counterEncoder,  nullptr,        nullptr, counterRender, nullptr, nullptr};
  textApp    = {"notes",    "Notes",   notesEnter, notesEncoder, nullptr,        notesMessage, notesRender, nullptr, nullptr};
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
  if (strstr(json, "\"theme\"")) {   // phone toggles the light/dark theme
    displaySetTheme(strstr(json, "\"light\"") != nullptr);
    if (g_active && g_active->render) g_active->render();
    return;
  }
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
