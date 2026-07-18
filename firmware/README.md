# Genesis Mini firmware contract

Generate the first Arduino implementation in Axiometa Studio because it knows the
Genesis Mini port diagram and can also generate a standalone, board-hosted mobile
dashboard. Do not freeze GPIO numbers until the actual port assignment is
confirmed in Studio.

## Responsibilities

- run a fixed frame loop for two 160×80 ST7735S displays;
- decode rotary direction and button press;
- play short PWM confirmation tones (or I2S audio with MAX98357A);
- store the last valid layout in ESP32 preferences;
- expose local Wi-Fi endpoints matching the shared TypeScript protocol.

## Minimal API

```text
GET  /status  -> { name, firmware, activeWidget, online }
POST /layout  <- { version, activeWidget, brightness, volume }
WS   /events  -> encoder and telemetry messages (optional for MVP)
```

## Bring-up order

1. Generate a one-display sketch in Axiometa Studio and save the working pin map.
2. Add the rotary example and verify direction/button events over Serial.
3. Add the second display using the second port's CS/RST mapping.
4. Add the HTTP endpoint only after the offline encoder demo is stable.

The product pages show sample pins for a single module in a particular port; those
examples are not sufficient evidence for the final two-display mapping.

The four-port MVP assigns two displays, the rotary encoder, and the passive buzzer.
Speech capture and playback stay on the phone. A microphone plus amplified speaker
would exceed the Mini's port count unless one uses an extender or another bus.
