#!/usr/bin/env python3
"""HatOS <-> Spotify bridge.

Runs on a machine that shares a WiFi network with the hat (e.g. a phone hotspot).
Reads your live Spotify now-playing and pushes it to the hat over WebSocket, streams
the album cover as raw pixels, and relays the encoder (play/pause/skip/volume) back
to Spotify. Audio plays on your phone; the hat is the display + remote.

Config via environment:
  SPOTIFY_CLIENT_ID   your Spotify app's Client ID (developer.spotify.com/dashboard)
  HAT_WS              hat WebSocket URL (default ws://172.20.10.2:81)
Redirect URI to register on the Spotify app: http://127.0.0.1:8888/callback

Note: the hat's tiny JSON parser expects COMPACT JSON (no spaces after colons),
so every send uses separators=(",", ":").
"""
import json, os, threading, time, io, urllib.request, sys
import spotipy
from spotipy.oauth2 import SpotifyPKCE
import websocket  # websocket-client
from PIL import Image

CLIENT_ID = os.environ.get("SPOTIFY_CLIENT_ID")
if not CLIENT_ID:
    sys.exit("Set SPOTIFY_CLIENT_ID (your Spotify app Client ID). See README.")
REDIRECT = "http://127.0.0.1:8888/callback"
SCOPE    = "user-read-playback-state user-modify-playback-state user-read-currently-playing"
HAT_WS   = os.environ.get("HAT_WS", "ws://172.20.10.2:81")
ART      = 64

auth = SpotifyPKCE(client_id=CLIENT_ID, redirect_uri=REDIRECT, scope=SCOPE,
                   open_browser=True, cache_path=os.path.expanduser("~/.hatspotify_cache"))
def new_client():
    return spotipy.Spotify(auth_manager=auth, requests_timeout=8, retries=0)
spx = {"sp": new_client()}
print(">>> Authorizing with Spotify (browser opens if needed)...", flush=True)
me = spx["sp"].current_user(); print(f">>> Logged in as {me.get('display_name')}", flush=True)

cur = {"ws": None}
st  = {"playing": False}
_last_art = {"url": None}
vlock = threading.Lock()
vpend = {"val": None, "ts": 0.0}
vuser = {"val": None, "ts": 0.0}

def compact(obj):
    return json.dumps(obj, separators=(",", ":"))   # hat parser needs no spaces

def safe_send(obj_or_bytes, binary=False):
    ws = cur["ws"]
    if ws is None: return False
    try:
        ws.send(obj_or_bytes, opcode=websocket.ABNF.OPCODE_BINARY) if binary else ws.send(compact(obj_or_bytes))
        return True
    except Exception:
        return False

def send_art(url):
    if not url or url == _last_art["url"]: return
    try:
        raw = urllib.request.urlopen(url, timeout=6).read()
        im = Image.open(io.BytesIO(raw)).convert("RGB").resize((ART, ART)); px = im.load()
        buf = bytearray(ART*ART*2); i = 0
        for y in range(ART):
            for x in range(ART):
                r, g, b = px[x, y]
                v = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)   # RGB565, hi-byte first
                buf[i] = (v >> 8) & 0xFF; buf[i+1] = v & 0xFF; i += 2
        if safe_send(bytes(buf), binary=True):
            _last_art["url"] = url; print(f"art sent ({len(buf)} bytes)", flush=True)
    except Exception as e:
        print("art err:", e, flush=True)

def vol_applier():
    while True:
        time.sleep(0.1)
        with vlock: v, ts = vpend["val"], vpend["ts"]
        if v is not None and (time.time() - ts) > 0.2:      # debounced: apply once the knob settles
            try: spx["sp"].volume(int(v)); print("apply volume ->", v, flush=True)
            except Exception as e: print("vol err:", e, flush=True)
            with vlock:
                if vpend["ts"] == ts: vpend["val"] = None

def poll_loop():
    last = None; errs = 0
    while True:
        if cur["ws"] is None:
            time.sleep(0.4); continue
        try:
            pb = spx["sp"].current_playback(); errs = 0
            if pb and pb.get("item"):
                it = pb["item"]
                vol = (pb.get("device") or {}).get("volume_percent") or 0
                with vlock:
                    if vuser["val"] is not None and (time.time() - vuser["ts"]) < 4:
                        vol = vuser["val"]
                payload = {"title": it["name"][:38],
                           "artist": (", ".join(a["name"] for a in it["artists"]))[:38],
                           "positionMs": pb.get("progress_ms") or 0,
                           "durationMs": it.get("duration_ms") or 0,
                           "isPlaying": bool(pb.get("is_playing")), "volume": vol}
                st["playing"] = payload["isPlaying"]
                if safe_send({"type": "music.state", "payload": payload}):
                    tag = f'{payload["title"]} — {payload["artist"]} [{"play" if payload["isPlaying"] else "pause"}]'
                    if tag != last: print("push:", tag, flush=True); last = tag
                    imgs = (it.get("album") or {}).get("images") or []
                    if imgs: send_art(imgs[-1].get("url"))
            else:
                if last != "(nothing)": print("push: (nothing playing)", flush=True); last = "(nothing)"
        except Exception as e:
            errs += 1; print("poll err:", e, flush=True)
            if errs >= 2:                                    # stale session -> rebuild
                spx["sp"] = new_client(); errs = 0; print(">>> rebuilt Spotify client", flush=True)
        time.sleep(1.0)

def on_open(ws):
    cur["ws"] = ws; _last_art["url"] = None
    print(">>> Hat connected.", flush=True)
    try: ws.send(compact({"type": "app.setActive", "payload": {"id": "music"}}))
    except Exception: pass

def on_close(ws, *a):
    cur["ws"] = None; print("hat disconnected; will reconnect...", flush=True)

def on_message(ws, message):
    try: m = json.loads(message)
    except Exception: return
    if m.get("type") != "music.command": return
    act = m.get("payload", {}).get("action"); val = m.get("payload", {}).get("value")
    try:
        if act == "playpause": (spx["sp"].pause_playback() if st["playing"] else spx["sp"].start_playback())
        elif act == "next": spx["sp"].next_track()
        elif act == "prev": spx["sp"].previous_track()
        elif act == "volume" and val is not None:
            with vlock:
                vpend["val"] = int(val); vpend["ts"] = time.time()
                vuser["val"] = int(val); vuser["ts"] = time.time()
        print("knob:", act, val if val is not None else "", flush=True)
    except Exception as e: print("cmd err:", e, flush=True)

threading.Thread(target=poll_loop, daemon=True).start()
threading.Thread(target=vol_applier, daemon=True).start()
print(f">>> Bridging to {HAT_WS}", flush=True)
while True:
    try:
        ws = websocket.WebSocketApp(HAT_WS, on_open=on_open, on_message=on_message,
                                    on_close=on_close, on_error=lambda w, e: None)
        ws.run_forever(ping_interval=20, ping_timeout=8)
    except Exception as e:
        print("loop err:", e, flush=True)
    cur["ws"] = None
    time.sleep(2)
