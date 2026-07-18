# HatOS platform — design spec

Date: 2026-07-18
Status: approved (architecture-level)
Scope: one high-level architecture spec covering all five subsystems. Each
subsystem gets its own detailed implementation plan afterward.

## 1. Product

A pocket/wearable platform on the Axiometa Genesis Mini: a **screen**, a **rotary
encoder**, and a **speaker**. It pairs to a phone over Bluetooth LE and becomes a
small always-on companion display + alert device. Apps are software; the hardware
is a general platform.

Launch apps: a **Spotify** control + now-playing surface, and a **Maps**
turn-by-turn view with a 45°-tilted directional arrow.

## 2. Core architecture decision

**Phone = brain, device = thin client, with Wi-Fi asset fetch (Approach 3).**

- The phone runs all app logic, API integrations, and secrets, and pushes
  render-ready state to the device over BLE.
- The device renders that state and sends encoder events back.
- Data too big for BLE (album art images, alert/voice audio clips) is **not** sent
  over BLE. The phone hands the device a `{url}` and the device fetches the asset
  over Wi-Fi (which is why the phone provisions Wi-Fi credentials).
- Rejected alternatives: pure thin-client (BLE can't carry images/audio);
  device-autonomous (OAuth/secrets on an ESP32 is painful and insecure).

## 3. Hard hardware constraints (verified on-device)

These shaped the design and must not be re-litigated:

- **The Genesis Mini's ESP32-S3 has Bluetooth LE only — no Classic BT (BR/EDR).**
  Therefore **A2DP is impossible**: the device can never be a Bluetooth speaker
  receiving the phone's audio. Confirmed by chip-detect ("BT 5 (LE)").
- Because of that, and Spotify DRM/ToS, **Spotify audio never comes out of the
  device speaker.** Spotify is a control + now-playing surface only; its audio
  plays on the phone.
- The **speaker is for alerts only** in the MVP: built-in chimes, notification
  sounds, turn cues, and optionally a fetched short voice/TTS clip. On-device I²S
  audio via the MAX98357A is already proven working.
- One screen (the verified ST7735S 160×80 on port 4). The earlier two-display plan
  is dropped.
- Full port→GPIO map and per-module wiring live in [`firmware/HARDWARE.md`](../../../firmware/HARDWARE.md).

## 4. Subsystems

### A. Transport & provisioning
- Device = BLE peripheral, phone = central. GATT services:
  - **Provisioning** — phone writes Wi-Fi SSID/password; device notifies join status.
  - **Control** — bidirectional JSON messages (app state, config, now-playing,
    maneuvers), length-prefixed and chunked across the negotiated BLE MTU.
  - **Input** — device notifies encoder turn/press events to the phone.
- Pairing flow: scan → connect → bond (persists) → phone pushes Wi-Fi creds →
  device joins Wi-Fi → status back to phone.
- Protocol shapes extend the existing `packages/protocol` TypeScript types;
  firmware mirrors the same message shapes. Heavy assets travel as a `{url}` fetched
  over Wi-Fi, never over BLE.

### B. Device runtime & app framework
- Render/input loop: encoder ISR → active app → render to ST7735; BLE and Wi-Fi I/O
  are handled off the render path.
- **App interface** (the boundary that makes future OTA possible without rework):
  each app implements `onEnter`, `onExit`, `onEncoder`, `onButton`,
  `onMessage(state)`, `render(gfx)`, `onTick`.
- **Registry + manifest** of built-in apps (id, name, icon). A built-in **launcher**
  ("home") app: encoder scrolls the enabled app list, press opens, long-press
  returns home.
- **Persistence:** active app + per-app config + last state in NVS/Preferences, so
  the device keeps working across phone disconnects.
- **Audio service:** a shared I²S alert player (built-in chimes + optional fetched
  clip) that any app can call.

### C. Phone companion
- Built on the existing Expo app (`apps/mobile`). Screens: pair device, provision
  Wi-Fi, app catalog (enable/reorder/configure), per-app control.
- App bridges run here: Spotify Web API (OAuth on phone) and navigation/directions
  → maneuver events.
- Pushes live state + config over BLE; hands asset URLs to the device.
- Keeps the "Demo Hat" simulator as the always-works fallback.
- **Build cost:** BLE in React Native requires an Expo **dev build**
  (`react-native-ble-plx`), not stock Expo Go.

### D. Spotify app
- Phone polls the Spotify Web API for now-playing: track, artist, album, art URL,
  `is_playing`, progress. Text/state pushed over BLE; art URL fetched by the device
  over Wi-Fi.
- Encoder = remote: turn → volume/scrub, press → play/pause. Stretch: press-hold →
  skip.
- Device renders a now-playing card: title, artist, progress bar, small album art
  (when Wi-Fi fetch succeeds).

### E. Maps app
- Phone computes the next maneuver `{turnType, distanceMeters, street, bearing}` and
  pushes it over BLE.
- Device renders a bold **45°-tilted arrow/chevron** for the turn + distance + street
  name, with an optional turn chime through the speaker.
- Encoder cycles upcoming steps / overview.
- Exact arrow visuals are designed when this app is built (mockups warranted).

## 5. Error handling, offline & demo fallback
- **Phone disconnects:** device keeps last state, shows a "disconnected" glyph,
  encoder still drives local UI.
- **Wi-Fi down:** skip asset fetch → text-only rendering (no art); alerts fall back
  to built-in chimes.
- **BLE:** auto-reconnect; bonding persists across power cycles.
- **Demo resilience:** Demo Hat simulator + scripted now-playing/maneuver data if
  live APIs fail.

## 6. Build order (vertical slices)
1. BLE link + protocol echo (encoder events → phone; phone pushes text → screen).
2. App framework + launcher on device (2 stub apps, encoder switching).
3. Wi-Fi provisioning over BLE + device joins + asset fetch (album art).
4. Spotify app end-to-end.
5. Maps app end-to-end.
6. Persistence + reconnect + demo polish.

## 7. Non-goals for MVP (YAGNI)
- No OTA app install (design the app boundary for it; do not build it).
- No on-device music streaming or A2DP (impossible on this chip).
- No on-device secrets/OAuth.
- Single screen.
- No on-device TTS synthesis (fetch a clip or use built-in chimes).

## 8. Open items for per-subsystem specs
- Exact GATT UUIDs, MTU/chunking format, and message schema in `packages/protocol`.
- Spotify OAuth flow + token refresh handling on the phone.
- Directions/nav data source for the Maps app (OS nav vs a directions API).
- Maps arrow visual language (mockups).
- Asset cache/eviction policy on the device.
