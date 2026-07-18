#include <NimBLEDevice.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "ble_link.h"
#include "line_framer.h"

static const char* SVC_UUID  = "6e6f7400-b5a3-f393-e0a9-e50e24dcca9e";
static const char* RX_UUID   = "6e6f7401-b5a3-f393-e0a9-e50e24dcca9e"; // phone -> hat (write)
static const char* TX_UUID   = "6e6f7402-b5a3-f393-e0a9-e50e24dcca9e"; // hat -> phone (notify)

static NimBLECharacteristic* g_tx = nullptr;
static void (*g_onMessage)(const char*) = nullptr;
static bool g_connected = false;
static LineFramer g_framer;
// Serializes bleSend() so chunks from concurrent callers (loop() task and
// the NimBLE host task, e.g. ping->pong reply from RxCb::onWrite) never
// interleave on the wire.
static SemaphoreHandle_t g_txMutex = nullptr;

class RxCb : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c, NimBLEConnInfo&) override {
    std::string v = c->getValue();
    g_framer.push((const uint8_t*)v.data(), v.size(), g_onMessage);
  }
};
class SrvCb : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* s, NimBLEConnInfo&) override { g_connected = true; }
  void onDisconnect(NimBLEServer* s, NimBLEConnInfo&, int) override {
    g_connected = false; NimBLEDevice::startAdvertising();
  }
};

void bleBegin(void (*onMessage)(const char* json)) {
  g_onMessage = onMessage;
  g_txMutex = xSemaphoreCreateMutex();
  NimBLEDevice::init("HatOS");
  NimBLEDevice::setMTU(247);
  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(new SrvCb());
  NimBLEService* svc = server->createService(SVC_UUID);
  NimBLECharacteristic* rx =
      svc->createCharacteristic(RX_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  rx->setCallbacks(new RxCb());
  g_tx = svc->createCharacteristic(TX_UUID, NIMBLE_PROPERTY::NOTIFY);
  svc->start();
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(SVC_UUID);
  adv->setName("HatOS");
  NimBLEDevice::startAdvertising();
}

bool bleConnected() { return g_connected; }

void bleSend(const char* json) {
  if (!g_connected || !g_tx) return;
  if (g_txMutex) xSemaphoreTake(g_txMutex, portMAX_DELAY);
  std::string line = std::string(json) + "\n";
  size_t mtu = 20;  // fixed conservative chunk size (guaranteed-minimum ATT payload); not negotiated MTU-aware
  for (size_t i = 0; i < line.size(); i += mtu) {
    std::string part = line.substr(i, mtu);
    g_tx->setValue((const uint8_t*)part.data(), part.size());
    g_tx->notify();
    delay(3);
  }
  if (g_txMutex) xSemaphoreGive(g_txMutex);
}
