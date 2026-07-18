# HatOS hackathon plan

## Product pitch

HatOS turns a cap into a configurable social interface: private information faces
the wearer, while a second display communicates status, captions, or personality
to everyone else. A rotary encoder makes it usable without taking out a phone.

## Architecture decision

The Genesis Mini is the device controller. It reads the encoder, drives both
ST7735S displays and the buzzer/speaker, and keeps the last layout locally. The
phone is only a companion for choosing widgets and supplying network-powered AI.

Cloud AI is a narrow optional layer: a Modal web endpoint holds secrets and calls
Linkup for current, sourced web answers. The phone calls Modal; the ESP32 never
receives API credentials. Keep a scripted response in the phone for demo resilience.

Use Axiometa Studio's **Standalone over WiFi** mode as the primary hardware path.
Studio says it can generate a webpage hosted by the board which any phone on the
same network can control, with no app installation. Ask Axie to build both the
Arduino firmware and a simple mobile dashboard. Keep the Expo Go companion as the
polished simulated demo and fallback. Add BLE only if local Wi-Fi proves
impossible; BLE in React Native requires an Expo development build rather than
stock Expo Go.

## MVP in build order

1. **Guaranteed demo:** companion app selects a widget and previews both screens.
2. **One real display:** firmware renders the selected widget on one AX22-0034.
3. **Physical controls:** encoder changes widget; push confirms; buzzer chirps.
4. **Second display:** share the SPI bus, with a separate chip-select/reset mapping
   confirmed from the Genesis Mini port schematic.
5. **Phone sync:** generate a board-hosted mobile dashboard in Studio; keep the
   protocol endpoints below if Studio's generated architecture allows them.
6. **One AI moment:** phone speech-to-text sends a question to Modal; Modal uses
   Linkup and returns a short sourced answer. The ESP32 receives display text only.

## What to fake safely

- Keep “Demo Hat” selectable even after hardware transport exists.
- If networking fails, use the encoder to cycle through preloaded widget data.
- If the second display fails, mirror it in the phone preview and demo one physical
  display; the product story still lands.
- AI output can be a scripted three-response sequence triggered from the phone.

## Suggested three-minute demo

1. Show the two physical viewing directions and explain private versus public.
2. Pick “Mood Badge” in the companion and sync it.
3. Turn the encoder to “Pitch Timer”; press it and let the buzzer confirm.
4. Speak a sentence into the phone; show a concise caption/status on the outward
   display and a private prompt on the wearer display.
5. End by showing that widgets are software and the hat is a general platform.

## Hardware checkpoints

- The Mini has four AX22 ports. Two displays + encoder + microphone + speaker is
  five AX22 modules, so the MVP uses the phone microphone and phone speaker. Move
  audio onto the hat only with a port extender or a confirmed non-AX22 connection.
- Confirm whether the two chosen AX22 ports expose distinct display CS/RST pins.
- Confirm the board revision: the Mini product page lists four AX22 ports, while
  the starter-kit copy currently says eight.
- A passive buzzer can chirp but cannot reproduce speech. Spoken AI uses phone
  audio for the MVP; a later hat version can use the MAX98357A plus speaker.
- The 0.96-inch display is 160×80 with a roughly 21.7×10.8 mm active area. Without
  optics, mount the wearer-facing screen at a normal focus distance or use it as a
  glance-down display rather than directly in front of the eye.

## Studio prompt to try

> I have a Genesis Mini with two AX22-0034 ST7735S displays, one AX22-0003 rotary
> encoder, and a passive buzzer. Build an offline-first wearable interface. One
> display faces the wearer and one faces other people. The encoder cycles widgets,
> its button activates the selected widget, and the buzzer confirms actions. Also
> create a mobile browser dashboard hosted by the board over Wi-Fi where I can set
> each display's short text, choose a widget, and change brightness. Persist the
> latest state so the encoder and displays keep working when the phone disconnects.
> First show me the detected port diagram and flag any SPI or pin conflict between
> the two displays before generating code.

## Definition of done

- Phone demo starts in under 30 seconds and needs no login.
- Both screen states are visible in the app and at least one is visible on hardware.
- Encoder selection works without the phone.
- Demo continues if Wi-Fi or AI is unavailable.
