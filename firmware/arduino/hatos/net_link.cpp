#include <WiFi.h>
#include <WebSocketsServer.h>
#include "net_link.h"
#include "ble_link.h"

static WebSocketsServer ws(81);
static void (*g_onMsg)(const char*) = nullptr;

static void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t len) {
  if (type == WStype_TEXT && g_onMsg) {
    static char buf[512];
    size_t n = (len < sizeof(buf) - 1) ? len : sizeof(buf) - 1;
    memcpy(buf, payload, n); buf[n] = 0;
    g_onMsg(buf);                 // one JSON message per WS text frame
  }
}

void netBegin(void (*onMessage)(const char* json)) {
  g_onMsg = onMessage;
  WiFi.mode(WIFI_AP);
  WiFi.softAP("HatOS", "hatos1234");            // SSID, WPA2 password (>=8 chars)
  IPAddress ip = WiFi.softAPIP();
  Serial.printf("SoftAP up: SSID=HatOS pass=hatos1234 ws://%s:81\n", ip.toString().c_str());
  ws.begin();
  ws.onEvent(onWsEvent);
}
void netLoop() { ws.loop(); }
void netBroadcast(const char* json) { ws.broadcastTXT((char*)json); }
void hatSend(const char* json) { bleSend(json); netBroadcast(json); }
