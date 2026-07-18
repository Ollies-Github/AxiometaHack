# HatOS — Axiometa Hackathon Wearable

HatOS is a two-sided wearable display built around the Axiometa Genesis Mini. One
screen faces the wearer, one faces the room, and a rotary encoder controls the
active widget. A phone companion chooses widgets and configures what each screen
shows.

## Repository

- [`apps/mobile`](apps/mobile) — Expo companion app; runs immediately in Expo Go
- [`packages/protocol`](packages/protocol) — shared device messages and widget types
- [`services/ai`](services/ai) — Modal endpoint backed by Linkup search
- [`firmware`](firmware) — ESP32 integration contract and firmware plan
- [`cad`](cad/README.md) — printable servo-powered hat mount
- [`docs/HACKATHON_PLAN.md`](docs/HACKATHON_PLAN.md) — build order, demo script, and fallback plan

## Run the companion

```bash
npm install
npm run mobile
```

Scan the QR code with Expo Go. The app starts against a simulated “Demo Hat,” so
the full widget-selection demo works before the electronics are ready.
