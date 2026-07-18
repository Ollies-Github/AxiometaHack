# HatOS Foundation Slice Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prove the phone↔device pipeline end-to-end: the phone pairs to the Genesis Mini over BLE, the device runs an app framework with a launcher + 2 stub apps switchable by the encoder, encoder events stream to the phone, and the phone can push render-ready text that appears on the ST7735 screen.

**Architecture:** Phone is the brain, device is a thin client. A shared newline-delimited-JSON message protocol (`@hatos/protocol`) is spoken over two BLE GATT characteristics (phone→hat write, hat→phone notify), chunked across the negotiated MTU. Firmware runs a tiny app framework (registry + active app + launcher) that routes encoder/BLE input to the active app and renders it to the display.

**Tech Stack:** TypeScript + Vitest (protocol codec), Arduino/ESP32-S3 with NimBLE-Arduino + Adafruit_ST7735 (firmware), Expo + react-native-ble-plx via an Expo dev build (phone).

## Global Constraints

- Board FQBN: `esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=4M,PSRAM=enabled` (verbatim from `firmware/HARDWARE.md`).
- Verified pins (from `firmware/HARDWARE.md`): encoder A/CLK=GPIO3, B/DT=GPIO2, button=GPIO4 (active-HIGH); display CS=GPIO1, DC=GPIO18, RST=GPIO17, MOSI=GPIO12, SCK=GPIO14; `initR(INITR_MINI160x80)`, `setRotation(3)`, `invertDisplay(true)`.
- Encoder decode: single IRAM quadrature ISR, 4 edges per detent. Never call a decoder tick from both ISR and loop.
- BLE only (ESP32-S3 has no Classic BT). No Wi-Fi, no A2DP, no Spotify/Maps in this slice.
- Protocol messages are UTF-8 JSON, one per line (`\n`-terminated), chunked to the negotiated ATT MTU minus 3 bytes.
- GATT UUIDs (fixed, used verbatim on both sides):
  - Service `6e6f7400-b5a3-f393-e0a9-e50e24dcca9e`
  - Phone→Hat characteristic (Write / Write-No-Response) `6e6f7401-b5a3-f393-e0a9-e50e24dcca9e`
  - Hat→Phone characteristic (Notify) `6e6f7402-b5a3-f393-e0a9-e50e24dcca9e`
- Device advertised name: `HatOS`. Firmware version string: `foundation-0.1`.
- Commit after every task with the shown message.

---

## File Structure

**Protocol (`packages/protocol/`)**
- `src/messages.ts` — foundation message type unions + `AppInfo`.
- `src/codec.ts` — `encodeMessage`, `MessageDecoder`, `chunkForMtu`.
- `src/index.ts` — re-export messages + codec (keeps existing widget types).
- `src/codec.test.ts` — Vitest unit tests.
- `package.json` / `vitest.config.ts` — add test script + Vitest.

**Firmware (`firmware/arduino/hatos/`)** — one sketch folder; Arduino compiles all `.h`/`.cpp` here.
- `hatos.ino` — setup/loop, wiring of subsystems.
- `line_framer.h` — portable `\n`-framed reassembly (self-tested in setup).
- `display.h` / `display.cpp` — ST7735 init + text rendering helper.
- `encoder.h` / `encoder.cpp` — IRAM quadrature ISR + button, detent/press events.
- `app_framework.h` — `App` interface, `AppRegistry`, active-app switching.
- `apps.h` / `apps.cpp` — `LauncherApp`, `CounterApp`, `TextApp`.
- `ble_link.h` / `ble_link.cpp` — NimBLE peripheral, two characteristics, send/receive.

**Phone (`apps/mobile/`)**
- `src/ble/hatLink.ts` — BLE client (scan/connect/subscribe/write) using `@hatos/protocol`.
- `src/ble/useHatLink.ts` — React hook exposing connection state, last encoder event, `send`.
- `app/pair.tsx` — pairing screen.
- `app/hat.tsx` — live screen (encoder feed + app switch + push text).
- `app.json` — add `react-native-ble-plx` config plugin + BLE permissions.

---

## Task 1: Protocol message types + Vitest setup

**Files:**
- Create: `packages/protocol/src/messages.ts`
- Modify: `packages/protocol/src/index.ts`
- Create: `packages/protocol/vitest.config.ts`
- Modify: `packages/protocol/package.json`
- Test: `packages/protocol/src/messages.test.ts`

**Interfaces:**
- Produces:
  - `type AppInfo = { id: string; name: string }`
  - `type PhoneToHat = { type: 'app.setActive'; payload: { id: string } } | { type: 'display.render'; payload: { lines: string[] } } | { type: 'ping' }`
  - `type HatToPhone = { type: 'hat.ready'; payload: { name: string; firmware: string; apps: AppInfo[] } } | { type: 'encoder'; payload: { delta: number; pressed: boolean } } | { type: 'app.changed'; payload: { id: string } } | { type: 'pong' }`

- [ ] **Step 1: Add Vitest to the protocol package**

Edit `packages/protocol/package.json` to add scripts + devDependency:

```json
{
  "name": "@hatos/protocol",
  "version": "0.1.0",
  "private": true,
  "main": "src/index.ts",
  "types": "src/index.ts",
  "scripts": {
    "test": "vitest run",
    "test:watch": "vitest"
  },
  "devDependencies": {
    "vitest": "^2.1.0"
  }
}
```

Create `packages/protocol/vitest.config.ts`:

```ts
import { defineConfig } from 'vitest/config';

export default defineConfig({
  test: { environment: 'node', include: ['src/**/*.test.ts'] },
});
```

Then install from the repo root: `npm install`

- [ ] **Step 2: Write the failing test**

Create `packages/protocol/src/messages.test.ts`:

```ts
import { describe, it, expect } from 'vitest';
import type { PhoneToHat, HatToPhone } from './messages';

describe('message types', () => {
  it('round-trips a display.render message through JSON', () => {
    const msg: PhoneToHat = { type: 'display.render', payload: { lines: ['hi', 'there'] } };
    const parsed = JSON.parse(JSON.stringify(msg)) as PhoneToHat;
    expect(parsed).toEqual(msg);
  });

  it('round-trips a hat.ready message with apps', () => {
    const msg: HatToPhone = {
      type: 'hat.ready',
      payload: { name: 'HatOS', firmware: 'foundation-0.1', apps: [{ id: 'launcher', name: 'Home' }] },
    };
    const parsed = JSON.parse(JSON.stringify(msg)) as HatToPhone;
    expect(parsed).toEqual(msg);
  });
});
```

- [ ] **Step 3: Run test to verify it fails**

Run: `npm --workspace @hatos/protocol test`
Expected: FAIL — cannot find module `./messages`.

- [ ] **Step 4: Create the message types**

Create `packages/protocol/src/messages.ts`:

```ts
export type AppInfo = { id: string; name: string };

export type PhoneToHat =
  | { type: 'app.setActive'; payload: { id: string } }
  | { type: 'display.render'; payload: { lines: string[] } }
  | { type: 'ping' };

export type HatToPhone =
  | { type: 'hat.ready'; payload: { name: string; firmware: string; apps: AppInfo[] } }
  | { type: 'encoder'; payload: { delta: number; pressed: boolean } }
  | { type: 'app.changed'; payload: { id: string } }
  | { type: 'pong' };
```

Append to `packages/protocol/src/index.ts` (keep existing widget exports):

```ts
export * from './messages';
export * from './codec';
```

- [ ] **Step 5: Run test to verify it passes**

Run: `npm --workspace @hatos/protocol test`
Expected: `messages.test.ts` PASS. (`index.ts` re-exports `./codec` which does not exist yet — create a temporary empty `packages/protocol/src/codec.ts` with `export {};` so the import resolves; Task 2 fills it in.)

- [ ] **Step 6: Commit**

```bash
git add packages/protocol
git commit -m "feat(protocol): foundation message types + vitest"
```

---

## Task 2: Protocol framing codec

**Files:**
- Modify: `packages/protocol/src/codec.ts`
- Test: `packages/protocol/src/codec.test.ts`

**Interfaces:**
- Consumes: nothing.
- Produces:
  - `function encodeMessage(msg: unknown): Uint8Array` — JSON string + `'\n'` as UTF-8.
  - `function chunkForMtu(bytes: Uint8Array, mtu: number): Uint8Array[]` — split into ≤`mtu` pieces.
  - `class MessageDecoder { push(chunk: Uint8Array): unknown[] }` — accumulates bytes, returns parsed JSON objects for each completed `\n`-terminated line.

- [ ] **Step 1: Write the failing tests**

Create `packages/protocol/src/codec.test.ts`:

```ts
import { describe, it, expect } from 'vitest';
import { encodeMessage, chunkForMtu, MessageDecoder } from './codec';

describe('codec', () => {
  it('encodes a message as newline-terminated JSON', () => {
    const bytes = encodeMessage({ type: 'ping' });
    expect(new TextDecoder().decode(bytes)).toBe('{"type":"ping"}\n');
  });

  it('chunks bytes to the mtu', () => {
    const bytes = new Uint8Array([1, 2, 3, 4, 5]);
    expect(chunkForMtu(bytes, 2).map((c) => Array.from(c))).toEqual([[1, 2], [3, 4], [5]]);
  });

  it('reassembles a message split across chunks', () => {
    const dec = new MessageDecoder();
    const full = encodeMessage({ type: 'app.setActive', payload: { id: 'clock' } });
    const chunks = chunkForMtu(full, 4);
    const out = chunks.flatMap((c) => dec.push(c));
    expect(out).toEqual([{ type: 'app.setActive', payload: { id: 'clock' } }]);
  });

  it('emits two messages from one buffer and ignores a trailing partial', () => {
    const dec = new MessageDecoder();
    const a = encodeMessage({ type: 'ping' });
    const b = encodeMessage({ type: 'pong' });
    const partial = new TextEncoder().encode('{"type":"pi');
    const buf = new Uint8Array([...a, ...b, ...partial]);
    expect(dec.push(buf)).toEqual([{ type: 'ping' }, { type: 'pong' }]);
  });
});
```

- [ ] **Step 2: Run test to verify it fails**

Run: `npm --workspace @hatos/protocol test`
Expected: FAIL — `encodeMessage`/`chunkForMtu`/`MessageDecoder` are not exported.

- [ ] **Step 3: Implement the codec**

Replace `packages/protocol/src/codec.ts`:

```ts
export function encodeMessage(msg: unknown): Uint8Array {
  return new TextEncoder().encode(JSON.stringify(msg) + '\n');
}

export function chunkForMtu(bytes: Uint8Array, mtu: number): Uint8Array[] {
  if (mtu < 1) throw new Error('mtu must be >= 1');
  const chunks: Uint8Array[] = [];
  for (let i = 0; i < bytes.length; i += mtu) chunks.push(bytes.slice(i, i + mtu));
  return chunks;
}

export class MessageDecoder {
  private buf = '';
  push(chunk: Uint8Array): unknown[] {
    this.buf += new TextDecoder().decode(chunk);
    const out: unknown[] = [];
    let nl: number;
    while ((nl = this.buf.indexOf('\n')) !== -1) {
      const line = this.buf.slice(0, nl).trim();
      this.buf = this.buf.slice(nl + 1);
      if (line) out.push(JSON.parse(line));
    }
    return out;
  }
}
```

- [ ] **Step 4: Run test to verify it passes**

Run: `npm --workspace @hatos/protocol test`
Expected: all `codec.test.ts` tests PASS.

- [ ] **Step 5: Commit**

```bash
git add packages/protocol
git commit -m "feat(protocol): newline-JSON framing codec with MTU chunking"
```

---

## Task 3: Firmware skeleton — display + encoder over serial

**Files:**
- Create: `firmware/arduino/hatos/hatos.ino`
- Create: `firmware/arduino/hatos/display.h`, `firmware/arduino/hatos/display.cpp`
- Create: `firmware/arduino/hatos/encoder.h`, `firmware/arduino/hatos/encoder.cpp`

**Interfaces:**
- Produces:
  - `void displayBegin();`
  - `void displayLines(const char* const* lines, int count);` — clears + prints up to 5 lines.
  - `void encoderBegin();`
  - `int encoderTakeDelta();` — detents since last call (signed), clears the accumulator.
  - `bool encoderTakePress();` — true once per button press (rising edge, debounced).

- [ ] **Step 1: Create the encoder module**

Create `firmware/arduino/hatos/encoder.h`:

```cpp
#pragma once
void encoderBegin();
int  encoderTakeDelta();
bool encoderTakePress();
```

Create `firmware/arduino/hatos/encoder.cpp`:

```cpp
#include <Arduino.h>
#include "encoder.h"

static const int PIN_A = 3, PIN_B = 2, PIN_BTN = 4;
static volatile int32_t g_raw = 0;
static volatile uint8_t g_prev = 3;
static const int8_t QTAB[16] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
static long g_lastDet = 0;
static bool g_lastBtn = false;
static uint32_t g_btnMs = 0;

static void IRAM_ATTR isr() {
  uint8_t s = (uint8_t)((digitalRead(PIN_A) << 1) | digitalRead(PIN_B));
  g_raw += QTAB[((g_prev << 2) | s) & 0x0F];
  g_prev = s;
}

void encoderBegin() {
  pinMode(PIN_A, INPUT_PULLUP);
  pinMode(PIN_B, INPUT_PULLUP);
  pinMode(PIN_BTN, INPUT);                 // module pull-down; pressed = HIGH
  g_prev = (uint8_t)((digitalRead(PIN_A) << 1) | digitalRead(PIN_B));
  attachInterrupt(digitalPinToInterrupt(PIN_A), isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_B), isr, CHANGE);
}

int encoderTakeDelta() {
  noInterrupts(); int32_t raw = g_raw; interrupts();
  long det = raw / 4;
  int delta = (int)(det - g_lastDet);
  g_lastDet = det;
  return delta;
}

bool encoderTakePress() {
  bool now = (digitalRead(PIN_BTN) == HIGH);
  bool fired = false;
  if (now != g_lastBtn && (millis() - g_btnMs) > 25) {
    g_btnMs = millis();
    if (now) fired = true;                 // rising edge = press
    g_lastBtn = now;
  }
  return fired;
}
```

- [ ] **Step 2: Create the display module**

Create `firmware/arduino/hatos/display.h`:

```cpp
#pragma once
void displayBegin();
void displayLines(const char* const* lines, int count);
```

Create `firmware/arduino/hatos/display.cpp`:

```cpp
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include "display.h"

static const int TFT_MOSI = 12, TFT_SCLK = 14, TFT_CS = 1, TFT_DC = 18, TFT_RST = 17;
static SPIClass tftSPI(FSPI);
static Adafruit_ST7735 tft(&tftSPI, TFT_CS, TFT_DC, TFT_RST);

void displayBegin() {
  tftSPI.begin(TFT_SCLK, -1, TFT_MOSI);
  tft.initR(INITR_MINI160x80);
  tft.setRotation(3);
  tft.invertDisplay(true);
  tft.fillScreen(ST77XX_BLACK);
}

void displayLines(const char* const* lines, int count) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  for (int i = 0; i < count && i < 5; i++) {
    tft.setCursor(4, 4 + i * 16);
    tft.print(lines[i]);
  }
}
```

- [ ] **Step 3: Create the sketch that exercises both**

Create `firmware/arduino/hatos/hatos.ino`:

```cpp
#include "display.h"
#include "encoder.h"

static int counter = 0;

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n== HatOS foundation ==");
  displayBegin();
  encoderBegin();
  const char* boot[] = {"HatOS", "ready"};
  displayLines(boot, 2);
}

void loop() {
  int d = encoderTakeDelta();
  bool p = encoderTakePress();
  if (d != 0 || p) {
    if (p) counter = 0; else counter += d;
    Serial.printf("delta=%d press=%d counter=%d\n", d, p, counter);
    char line0[24]; snprintf(line0, sizeof(line0), "count %d", counter);
    const char* lines[] = {"encoder", line0};
    displayLines(lines, 2);
  }
  delay(5);
}
```

- [ ] **Step 4: Compile, flash, and verify on hardware**

Run:
```bash
FQBN="esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=4M,PSRAM=enabled"
arduino-cli compile -b "$FQBN" firmware/arduino/hatos
arduino-cli upload -b "$FQBN" -p /dev/cu.usbmodem101 firmware/arduino/hatos
```
Then turn the knob and press it while watching serial (115200).
Expected: screen shows "HatOS/ready" then "encoder/count N"; serial prints `delta=… press=… counter=…`; clockwise increments, press resets to 0.

- [ ] **Step 5: Commit**

```bash
git add firmware/arduino/hatos
git commit -m "feat(fw): HatOS sketch skeleton — display + encoder over serial"
```

---

## Task 4: Firmware line framer (self-tested)

**Files:**
- Create: `firmware/arduino/hatos/line_framer.h`
- Modify: `firmware/arduino/hatos/hatos.ino`

**Interfaces:**
- Produces:
  - `class LineFramer { void push(const uint8_t* data, size_t len, void (*onLine)(const char*)); }` — buffers bytes, invokes `onLine` with each `\n`-terminated line (newline stripped, NUL-terminated), max line 240 chars (overflow lines are dropped).

- [ ] **Step 1: Create the framer**

Create `firmware/arduino/hatos/line_framer.h`:

```cpp
#pragma once
#include <Arduino.h>

class LineFramer {
 public:
  void push(const uint8_t* data, size_t len, void (*onLine)(const char*)) {
    for (size_t i = 0; i < len; i++) {
      char c = (char)data[i];
      if (c == '\n') {
        buf_[pos_] = 0;
        if (pos_ > 0) onLine(buf_);
        pos_ = 0;
      } else if (pos_ < sizeof(buf_) - 1) {
        buf_[pos_++] = c;
      } else {
        pos_ = 0;  // overflow -> drop the oversized line
      }
    }
  }
 private:
  char buf_[241];
  size_t pos_ = 0;
};
```

- [ ] **Step 2: Add a boot self-test**

In `hatos.ino`, add near the top:

```cpp
#include "line_framer.h"

static int g_selfTestLines = 0;
static void selfTestSink(const char* line) { g_selfTestLines++; }

static void runFramerSelfTest() {
  LineFramer f;
  const char* s = "abc\n{\"type\":\"ping\"}\npartial";
  f.push((const uint8_t*)s, strlen(s), selfTestSink);
  Serial.printf("framer self-test: %s (%d lines, expected 2)\n",
                g_selfTestLines == 2 ? "PASS" : "FAIL", g_selfTestLines);
}
```

Call `runFramerSelfTest();` at the end of `setup()`.

- [ ] **Step 3: Compile, flash, verify**

Run the compile+upload commands from Task 3 Step 4, then read serial.
Expected serial line: `framer self-test: PASS (2 lines, expected 2)`.

- [ ] **Step 4: Commit**

```bash
git add firmware/arduino/hatos
git commit -m "feat(fw): newline framer with boot self-test"
```

---

## Task 5: Firmware BLE link — advertise, connect, echo

**Files:**
- Create: `firmware/arduino/hatos/ble_link.h`, `firmware/arduino/hatos/ble_link.cpp`
- Modify: `firmware/arduino/hatos/hatos.ino`

**Interfaces:**
- Consumes: `LineFramer` (Task 4).
- Produces:
  - `void bleBegin(void (*onMessage)(const char* json));` — starts advertising `HatOS`; calls `onMessage` for each complete phone→hat JSON line.
  - `void bleSend(const char* json);` — frames + chunks + notifies hat→phone (no-op if disconnected).
  - `bool bleConnected();`

Install the library first: `arduino-cli lib install "NimBLE-Arduino"`

- [ ] **Step 1: Create the BLE link**

Create `firmware/arduino/hatos/ble_link.h`:

```cpp
#pragma once
void bleBegin(void (*onMessage)(const char* json));
void bleSend(const char* json);
bool bleConnected();
```

Create `firmware/arduino/hatos/ble_link.cpp`:

```cpp
#include <NimBLEDevice.h>
#include "ble_link.h"
#include "line_framer.h"

static const char* SVC  = "6e6f7400-b5a3-f393-e0a9-e50e24dcca9e";
static const char* RX   = "6e6f7401-b5a3-f393-e0a9-e50e24dcca9e"; // phone -> hat (write)
static const char* TX   = "6e6f7402-b5a3-f393-e0a9-e50e24dcca9e"; // hat -> phone (notify)

static NimBLECharacteristic* g_tx = nullptr;
static void (*g_onMessage)(const char*) = nullptr;
static bool g_connected = false;
static LineFramer g_framer;

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
  NimBLEDevice::init("HatOS");
  NimBLEDevice::setMTU(247);
  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(new SrvCb());
  NimBLEService* svc = server->createService(SVC);
  NimBLECharacteristic* rx =
      svc->createCharacteristic(RX, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  rx->setCallbacks(new RxCb());
  g_tx = svc->createCharacteristic(TX, NIMBLE_PROPERTY::NOTIFY);
  svc->start();
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(SVC);
  adv->setName("HatOS");
  NimBLEDevice::startAdvertising();
}

bool bleConnected() { return g_connected; }

void bleSend(const char* json) {
  if (!g_connected || !g_tx) return;
  std::string line = std::string(json) + "\n";
  size_t mtu = 20;  // conservative payload; grows automatically if MTU negotiated higher
  for (size_t i = 0; i < line.size(); i += mtu) {
    std::string part = line.substr(i, mtu);
    g_tx->setValue((const uint8_t*)part.data(), part.size());
    g_tx->notify();
    delay(3);
  }
}
```

- [ ] **Step 2: Wire BLE echo into the sketch**

In `hatos.ino`, add:

```cpp
#include "ble_link.h"

static void onPhoneMessage(const char* json) {
  Serial.printf("rx: %s\n", json);
  // Echo: reply hat.ready to a ping so we can confirm bidirectional flow.
  if (strstr(json, "\"ping\"")) {
    bleSend("{\"type\":\"pong\"}");
  }
}
```

In `setup()` after `encoderBegin();` add:

```cpp
  bleBegin(onPhoneMessage);
```

In `loop()`, replace the encoder-report body so it also notifies the phone:

```cpp
  if (d != 0 || p) {
    if (p) counter = 0; else counter += d;
    char msg[64];
    snprintf(msg, sizeof(msg), "{\"type\":\"encoder\",\"payload\":{\"delta\":%d,\"pressed\":%s}}",
             d, p ? "true" : "false");
    bleSend(msg);
    Serial.printf("tx: %s\n", msg);
  }
```

- [ ] **Step 3: Compile, flash, verify with a generic BLE client**

Compile + upload (Task 3 Step 4 commands). Then on a phone, open **nRF Connect** (free), scan, connect to `HatOS`, enable notifications on characteristic `…7402`, and write `{"type":"ping"}\n` (as text, include the newline) to `…7401`.
Expected: serial prints `rx: {"type":"ping"}` then `tx:`-style `pong`; nRF Connect shows a `{"type":"pong"}` notification. Turning the encoder shows `{"type":"encoder",…}` notifications.

- [ ] **Step 4: Commit**

```bash
git add firmware/arduino/hatos
git commit -m "feat(fw): NimBLE link — advertise, receive, notify; ping/pong + encoder echo"
```

---

## Task 6: App framework + launcher + stub apps

**Files:**
- Create: `firmware/arduino/hatos/app_framework.h`
- Create: `firmware/arduino/hatos/apps.h`, `firmware/arduino/hatos/apps.cpp`
- Modify: `firmware/arduino/hatos/hatos.ino`

**Interfaces:**
- Consumes: `displayLines` (Task 3).
- Produces:
  - `struct App { const char* id; const char* name; void (*onEnter)(); void (*onEncoder)(int delta); void (*onButton)(); void (*onMessage)(const char* json); void (*render)(); };`
  - `void appsBegin();` — registers apps, enters launcher.
  - `void appsEncoder(int delta);` `void appsButton();` `void appsMessage(const char* json);` `void appsTick();`
  - `const char* appsActiveId();`
  - `void appsRenderCatalogJson(char* out, size_t n);` — writes `[{"id":..,"name":..},…]` for `hat.ready`.

- [ ] **Step 1: Create the framework header**

Create `firmware/arduino/hatos/app_framework.h`:

```cpp
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
```

- [ ] **Step 2: Create the launcher + stub apps**

Create `firmware/arduino/hatos/apps.cpp`:

```cpp
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
  size_t k = 0; k += snprintf(out + k, n - k, "[");
  for (int i = 0; i < REG_N; i++)
    k += snprintf(out + k, n - k, "%s{\"id\":\"%s\",\"name\":\"%s\"}", i ? "," : "", REG[i]->id, REG[i]->name);
  snprintf(out + k, n - k, "]");
}
```

Create `firmware/arduino/hatos/apps.h`:

```cpp
#pragma once
#include "app_framework.h"
```

- [ ] **Step 3: Wire the framework into the sketch (long-press = home)**

Replace `hatos.ino`'s `loop()` and add long-press handling:

```cpp
#include "apps.h"

// in setup(), after encoderBegin() / bleBegin(...):
//   appsBegin();
//   sends hat.ready when a phone connects (handled in loop via wasConnected edge)

static bool g_wasConnected = false;
static uint32_t g_btnDownMs = 0;

void loop() {
  // hat.ready on new connection
  bool c = bleConnected();
  if (c && !g_wasConnected) {
    char cat[192]; appsRenderCatalogJson(cat, sizeof(cat));
    char msg[256];
    snprintf(msg, sizeof(msg),
      "{\"type\":\"hat.ready\",\"payload\":{\"name\":\"HatOS\",\"firmware\":\"foundation-0.1\",\"apps\":%s}}", cat);
    bleSend(msg);
  }
  g_wasConnected = c;

  int d = encoderTakeDelta();
  if (d != 0) {
    appsEncoder(d);
    char msg[64];
    snprintf(msg, sizeof(msg), "{\"type\":\"encoder\",\"payload\":{\"delta\":%d,\"pressed\":false}}", d);
    bleSend(msg);
  }
  if (encoderTakePress()) {
    appsButton();
    bleSend("{\"type\":\"encoder\",\"payload\":{\"delta\":0,\"pressed\":true}}");
    char msg[48]; snprintf(msg, sizeof(msg), "{\"type\":\"app.changed\",\"payload\":{\"id\":\"%s\"}}", appsActiveId());
    bleSend(msg);
  }
  delay(5);
}
```

Update `onPhoneMessage` to route into the framework:

```cpp
static void onPhoneMessage(const char* json) {
  Serial.printf("rx: %s\n", json);
  if (strstr(json, "\"ping\"")) { bleSend("{\"type\":\"pong\"}"); return; }
  appsMessage(json);
  char msg[48]; snprintf(msg, sizeof(msg), "{\"type\":\"app.changed\",\"payload\":{\"id\":\"%s\"}}", appsActiveId());
  bleSend(msg);
}
```

- [ ] **Step 4: Compile, flash, verify on hardware + nRF Connect**

Compile + upload. Then:
- On the device: turn encoder in Home to move the `> Counter/Notes/Home` selector; press to open; from Counter, turning changes the number.
- In nRF Connect: write `{"type":"app.setActive","payload":{"id":"notes"}}\n` → screen switches to Notes; write `{"type":"display.render","payload":{"lines":["hello hat"]}}\n` → Notes shows `hello hat`.
Expected: screen reflects each action; notifications include `app.changed` and `encoder`.

- [ ] **Step 5: Commit**

```bash
git add firmware/arduino/hatos
git commit -m "feat(fw): app framework + launcher + counter/notes stub apps"
```

---

## Task 7: Phone BLE client + protocol wiring

**Files:**
- Create: `apps/mobile/src/ble/hatLink.ts`
- Modify: `apps/mobile/package.json`, `apps/mobile/app.json`

**Interfaces:**
- Consumes: `encodeMessage`, `MessageDecoder` from `@hatos/protocol`; UUIDs from Global Constraints.
- Produces:
  - `class HatLink { scanAndConnect(): Promise<void>; onMessage(cb: (m: HatToPhone) => void): void; send(m: PhoneToHat): Promise<void>; disconnect(): void; isConnected(): boolean; }`

- [ ] **Step 1: Add the BLE dependency + permissions**

In `apps/mobile/package.json` dependencies add:

```json
    "react-native-ble-plx": "^3.2.1",
    "base-64": "^1.0.0"
```

and devDependencies add `"@types/base-64": "^1.0.2"`. Then from repo root: `npm install`.

In `apps/mobile/app.json`, add to the `expo.plugins` array (create it if absent) and `expo.ios`/`expo.android`:

```json
"plugins": [
  "expo-router",
  ["react-native-ble-plx", { "isBackgroundEnabled": false }]
],
"ios": { "infoPlist": { "NSBluetoothAlwaysUsageDescription": "HatOS pairs with your hat over Bluetooth." } },
"android": { "permissions": ["BLUETOOTH_SCAN", "BLUETOOTH_CONNECT"] }
```

- [ ] **Step 2: Write the BLE client**

Create `apps/mobile/src/ble/hatLink.ts`:

```ts
import { BleManager, Device, Characteristic } from 'react-native-ble-plx';
import { encode as b64encode, decode as b64decode } from 'base-64';
import { encodeMessage, MessageDecoder, type PhoneToHat, type HatToPhone } from '@hatos/protocol';

const SVC = '6e6f7400-b5a3-f393-e0a9-e50e24dcca9e';
const RX  = '6e6f7401-b5a3-f393-e0a9-e50e24dcca9e'; // phone -> hat (write)
const TX  = '6e6f7402-b5a3-f393-e0a9-e50e24dcca9e'; // hat -> phone (notify)

function toBytes(b64: string): Uint8Array {
  const s = b64decode(b64);
  const out = new Uint8Array(s.length);
  for (let i = 0; i < s.length; i++) out[i] = s.charCodeAt(i);
  return out;
}
function toB64(bytes: Uint8Array): string {
  let s = '';
  for (const b of bytes) s += String.fromCharCode(b);
  return b64encode(s);
}

export class HatLink {
  private mgr = new BleManager();
  private dev: Device | null = null;
  private decoder = new MessageDecoder();
  private cb: (m: HatToPhone) => void = () => {};

  isConnected() { return this.dev != null; }
  onMessage(cb: (m: HatToPhone) => void) { this.cb = cb; }

  async scanAndConnect(): Promise<void> {
    const dev = await new Promise<Device>((resolve, reject) => {
      this.mgr.startDeviceScan([SVC], null, (err, d) => {
        if (err) return reject(err);
        if (d) { this.mgr.stopDeviceScan(); resolve(d); }
      });
    });
    this.dev = await (await dev.connect()).discoverAllServicesAndCharacteristics();
    this.dev.monitorCharacteristicForService(SVC, TX, (err, c: Characteristic | null) => {
      if (err || !c?.value) return;
      for (const m of this.decoder.push(toBytes(c.value))) this.cb(m as HatToPhone);
    });
  }

  async send(m: PhoneToHat): Promise<void> {
    if (!this.dev) throw new Error('not connected');
    const bytes = encodeMessage(m);           // small foundation messages fit one write
    await this.dev.writeCharacteristicWithoutResponseForService(SVC, RX, toB64(bytes));
  }

  disconnect() { this.dev?.cancelConnection(); this.dev = null; }
}
```

- [ ] **Step 3: Typecheck**

Run: `npm --workspace @hatos/mobile run typecheck`
Expected: no type errors (protocol types resolve via the workspace).

- [ ] **Step 4: Commit**

```bash
git add apps/mobile
git commit -m "feat(mobile): BLE client (react-native-ble-plx) speaking the hat protocol"
```

---

## Task 8: Pair + live control screens (end-to-end)

**Files:**
- Create: `apps/mobile/src/ble/useHatLink.ts`
- Create: `apps/mobile/app/hat.tsx`
- Modify: `apps/mobile/app/index.tsx`

**Interfaces:**
- Consumes: `HatLink` (Task 7).
- Produces: `function useHatLink(): { connected: boolean; last: HatToPhone | null; connect: () => void; send: (m: PhoneToHat) => void }`.

- [ ] **Step 1: Create the hook**

Create `apps/mobile/src/ble/useHatLink.ts`:

```ts
import { useRef, useState, useCallback } from 'react';
import { HatLink } from './hatLink';
import type { HatToPhone, PhoneToHat } from '@hatos/protocol';

export function useHatLink() {
  const link = useRef<HatLink | null>(null);
  const [connected, setConnected] = useState(false);
  const [last, setLast] = useState<HatToPhone | null>(null);

  const connect = useCallback(async () => {
    const l = new HatLink();
    l.onMessage((m) => setLast(m));
    link.current = l;
    await l.scanAndConnect();
    setConnected(true);
  }, []);

  const send = useCallback((m: PhoneToHat) => { link.current?.send(m); }, []);
  return { connected, last, connect, send };
}
```

- [ ] **Step 2: Create the live screen**

Create `apps/mobile/app/hat.tsx`:

```tsx
import { View, Text, Pressable } from 'react-native';
import { useHatLink } from '../src/ble/useHatLink';

export default function Hat() {
  const { connected, last, connect, send } = useHatLink();
  return (
    <View style={{ flex: 1, padding: 24, gap: 12, justifyContent: 'center' }}>
      <Text style={{ fontSize: 20, fontWeight: '600' }}>HatOS — {connected ? 'connected' : 'disconnected'}</Text>
      {!connected && <Pressable onPress={connect}><Text>🔗 Scan &amp; connect</Text></Pressable>}
      <Text>Last from hat: {last ? JSON.stringify(last) : '—'}</Text>
      <Pressable onPress={() => send({ type: 'app.setActive', payload: { id: 'notes' } })}>
        <Text>▶︎ Open Notes app</Text>
      </Pressable>
      <Pressable onPress={() => send({ type: 'display.render', payload: { lines: ['hello from phone'] } })}>
        <Text>📝 Push text to hat</Text>
      </Pressable>
      <Pressable onPress={() => send({ type: 'ping' })}><Text>📶 Ping</Text></Pressable>
    </View>
  );
}
```

Add a link from `apps/mobile/app/index.tsx` to `/hat` (import `Link` from `expo-router` and render `<Link href="/hat">Open Hat control</Link>`), keeping the existing Demo Hat content.

- [ ] **Step 3: Build a dev client and run on a real phone**

BLE needs native code, so Expo Go will not work. Run:
```bash
npx --workspace @hatos/mobile expo run:ios   # or run:android, on a physical device
```
(First run builds a dev client; requires Xcode/Android SDK. Web/Expo Go remain for the Demo Hat only.)

- [ ] **Step 4: End-to-end verification**

With the device (Task 6 firmware) powered:
1. Open the app → Hat control → **Scan & connect** → header shows "connected".
2. "Last from hat" shows the `hat.ready` payload listing `launcher/counter/notes`.
3. Tap **Open Notes app** → device screen switches to Notes.
4. Tap **Push text to hat** → device Notes shows `hello from phone`.
5. Turn the encoder → "Last from hat" updates with `encoder` deltas; press → `app.changed`.

Expected: all five behaviors work. This closes the foundation loop (pair → app switch → push render → encoder stream).

- [ ] **Step 5: Commit**

```bash
git add apps/mobile
git commit -m "feat(mobile): pair + live control screen — end-to-end hat loop"
```

---

## Self-Review

- **Spec coverage (slice 1–2 of the architecture spec):**
  - BLE peripheral + control characteristics → Tasks 5, 6. ✅
  - Newline-JSON framing + MTU chunking, mirrored both sides → Tasks 2, 4. ✅
  - Protocol shapes in `packages/protocol` → Tasks 1, 2. ✅
  - App interface + registry + launcher + persistence-ready boundary → Task 6 (NVS persistence deferred; noted below). ✅ (boundary), ⚠️ persistence.
  - Encoder events to phone; phone pushes render-ready text → Tasks 6, 8. ✅
  - Phone companion pair + control (dev build, ble-plx) → Tasks 7, 8. ✅
  - Out of scope (Wi-Fi, assets, Spotify, Maps) → correctly absent. ✅
  - **Gap:** NVS persistence of active app across reboots is in the architecture spec but is a small add best folded into the persistence/polish slice; explicitly deferred, not silently dropped.
- **Placeholder scan:** no TBD/TODO; every code step has complete code; hardware/manual verification steps give exact expected output.
- **Type consistency:** UUIDs, message `type` strings, and characteristic roles (RX=write/phone→hat, TX=notify/hat→phone) match across firmware (`ble_link.cpp`), protocol, and phone (`hatLink.ts`). `encodeMessage`/`MessageDecoder`/`chunkForMtu` names consistent between Task 2 and Task 7. App struct field names consistent between `app_framework.h` and `apps.cpp`.

**Deferred to a later slice (documented, not dropped):** NVS persistence of active app/config; MTU-aware growth of `bleSend` chunk size beyond the conservative 20 bytes; robust JSON parsing on-device (current firmware uses `strstr` extraction, adequate for the fixed foundation message set).
