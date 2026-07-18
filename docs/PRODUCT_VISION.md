# Product vision

## What it is

A pocket/wearable **platform** built on the Genesis Mini: a **screen**, a **rotary
encoder**, and a **speaker**. It pairs to your phone over **Bluetooth (BLE)** and
becomes a small always-on companion display + audio device. The phone is the
control plane; the device is the surface.

## How it connects

1. **Bluetooth first.** The phone pairs to the device over BLE — no app store
   install of firmware, no typing on the device.
2. **Phone provisions Wi-Fi.** Over the BLE link the phone hands the device your
   Wi-Fi SSID + password (and any tokens), so the device gets internet without ever
   showing credentials on-screen or needing its own keyboard.
3. **Phone manages apps.** You pick which "apps" are installed / active on the
   device from the phone. Apps are software; the hardware is a general platform.
4. **Audio on-device.** The MAX98357A + speaker plays audio locally (verified
   working — see `firmware/HARDWARE.md`).

## Launch apps

### 1. Spotify player / controller
- Device shows **Now Playing**: track title, artist, and progress.
- Rotary encoder = transport: turn to scrub/volume, press to play/pause; (stretch:
  press-hold or double-press to skip).
- Playback/audio can come through the device speaker.
- Auth + Spotify Web API calls live on the phone; the device only receives display
  state + audio, never secrets (same trust model as the AI layer).

### 2. Maps / navigation
- A **45°-angle (tilted / driver's-eye) view** with big **directional arrows** —
  "turn left in 200 m" style — rather than a flat top-down map.
- Encoder can cycle upcoming steps or zoom.
- Turn-by-turn data comes from the phone over BLE; the device renders the arrow +
  next-maneuver text.

## Why this changes the earlier plan

The original HatOS plan (two displays, phone-side audio, Wi-Fi web dashboard) was
written before hardware bring-up. Findings that move us:

- **On-device audio is real.** The MAX98357A I²S amp works, so the speaker is a
  first-class output — audio no longer has to stay on the phone.
- **BLE is the primary link**, not a board-hosted Wi-Fi page. The phone provisions
  Wi-Fi over BLE, which is a cleaner setup story and lets the device be truly
  portable. (Note: BLE in React Native needs an Expo dev build, not stock Expo Go.)
- **One screen + speaker + encoder** is the core loop; the second display becomes
  optional rather than central.

See `firmware/HARDWARE.md` for the verified pin maps for every module used here.
