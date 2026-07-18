# Spotify bridge

Shows your live Spotify now-playing (title, artist, progress, album art) on the
HatOS device and relays the encoder back to Spotify (play/pause, skip, volume).
Audio plays on your phone as normal — the hat is a display + remote.

## How it fits

```
  Spotify Web API  ◄── HTTPS ──  this bridge  ── WebSocket ──►  hat (Music app)
       ▲                              │                              │
  your phone plays ──────────────────┘         encoder events ───────┘
```

The bridge and the hat must be on the **same WiFi** (a phone hotspot works well —
the hat is 2.4 GHz only, so enable "Maximize Compatibility"). The hat runs in STA
mode (see `firmware/arduino/hatos/wifi_secrets.h`) and prints its IP on boot.

## Setup

1. Create a Spotify app at <https://developer.spotify.com/dashboard>:
   - add Redirect URI **`http://127.0.0.1:8888/callback`**
   - enable **Web API**, then copy the **Client ID**.
   - Spotify **Premium** is required for playback control (skip/pause/volume).
2. Install deps: `pip install -r requirements.txt`
3. Run:
   ```bash
   export SPOTIFY_CLIENT_ID=your_client_id
   export HAT_WS=ws://<hat-ip>:81        # optional; default ws://172.20.10.2:81
   python spotify_bridge.py
   ```
   A browser opens once to log in. Then play a song in Spotify and it appears on
   the hat; turn the encoder for volume, press for play/pause, double-tap to skip.

## Notes

- Sends **compact** JSON (`separators=(",", ":")`) — the hat's minimal on-device
  parser expects no spaces after colons.
- Album art is fetched, resized to 64×64, converted to RGB565, and streamed to the
  hat as a binary WebSocket frame (the hat blits it — no image decoding on-device).
- Self-heals stale HTTP connections and auto-reconnects if the hat reboots.
- The OAuth token is cached at `~/.hatspotify_cache` (gitignored).
