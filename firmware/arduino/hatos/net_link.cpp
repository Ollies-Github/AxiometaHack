#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "net_link.h"
#include "ble_link.h"
#include "control_page.h"
#include "display.h"
#include "app_framework.h"
#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#endif

static WebSocketsServer ws(81);
static WebServer http(80);              // serves the control page
static void (*g_onMsg)(const char*) = nullptr;

static void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t len) {
  if (type == WStype_TEXT && g_onMsg) {
    static char buf[4096];   // large enough for a full teleprompter script
    size_t n = (len < sizeof(buf) - 1) ? len : sizeof(buf) - 1;
    memcpy(buf, payload, n); buf[n] = 0;
    g_onMsg(buf);                 // one JSON message per WS text frame
  } else if (type == WStype_BIN) {
    extern void musicSetArt(const uint8_t* data, size_t len);
    musicSetArt(payload, len);    // binary frame: 64x64 RGB565 album-art bitmap
  } else if (type == WStype_CONNECTED) {
    // Announce ourselves so the companion reflects the real device: the app
    // catalog, then the currently-active app.
    char cat[192]; appsRenderCatalogJson(cat, sizeof(cat));
    char msg[256];
    snprintf(msg, sizeof(msg),
      "{\"type\":\"hat.ready\",\"payload\":{\"name\":\"HatOS\",\"firmware\":\"foundation-0.1\",\"apps\":%s}}", cat);
    ws.sendTXT(num, msg);
    char act[80];
    snprintf(act, sizeof(act), "{\"type\":\"app.changed\",\"payload\":{\"id\":\"%s\"}}", appsActiveId());
    ws.sendTXT(num, act);
  }
}

void netBegin(void (*onMessage)(const char* json)) {
  g_onMsg = onMessage;
  IPAddress ip;
#ifdef WIFI_SSID
  // Station mode: join a real WiFi (needed for internet-backed apps like Spotify).
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("WiFi: joining \"%s\" ", WIFI_SSID);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) { delay(250); Serial.print("."); }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    ip = WiFi.localIP();
    Serial.printf("WiFi OK: joined \"%s\"  http://%s/  ws://%s:81\n",
                  WIFI_SSID, ip.toString().c_str(), ip.toString().c_str());
    String ips = ip.toString();
    const char* lines[] = {"WiFi OK", ips.c_str()};
    displayLines(lines, 2);
    delay(1800);
  } else {
    Serial.println("WiFi FAILED -> SoftAP fallback");
  }
#endif
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("HatOS", "hatos1234");
    ip = WiFi.softAPIP();
    Serial.printf("SoftAP up: SSID=HatOS pass=hatos1234  http://%s/  ws://%s:81\n",
                  ip.toString().c_str(), ip.toString().c_str());
  }
  ws.begin();
  ws.onEvent(onWsEvent);
  http.on("/", []() { http.send(200, "text/html", CONTROL_HTML); });
  http.onNotFound([]() { http.send(200, "text/html", CONTROL_HTML); });  // any path -> control page
  http.begin();
}
void netLoop() { ws.loop(); http.handleClient(); }
bool netIsConnected() { return WiFi.status() == WL_CONNECTED; }
void netBroadcast(const char* json) { ws.broadcastTXT((char*)json); }
void netBroadcastBin(const uint8_t* data, size_t len) { ws.broadcastBIN((uint8_t*)data, len); }
void hatSend(const char* json) { bleSend(json); netBroadcast(json); }
