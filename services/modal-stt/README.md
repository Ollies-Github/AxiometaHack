# HatOS voice — Modal STT

Fast Whisper speech-to-text on a warm Modal L4 GPU, behind a CORS-enabled
FastAPI endpoint. The phone/browser records mic audio and POSTs it straight
here; the transcript is pushed to the hat teleprompter.

```
  phone/browser mic ── multipart POST ──► Modal (faster-whisper, L4) ──► {text, ms}
                                                                            │
                                              transcript ─► hat teleprompter┘
```

## Deploy

```bash
pip install modal
modal setup                 # one-time browser auth
modal deploy stt.py         # prints a URL like https://<you>--hatos-stt-web.modal.run
```

Copy that URL into the app's **Voice** screen (the base URL — the app appends
`/transcribe`).

## Tuning

- **Model** (speed → accuracy): `distil-small.en`, `distil-large-v3` (default),
  `large-v3-turbo`. Override at deploy: `HATOS_STT_MODEL=large-v3-turbo modal deploy stt.py`.
- **Latency:** `min_containers=1` keeps one GPU warm so there's no cold start
  during a demo. It bills while up — set it back to `0` when you're done, or run
  `modal app stop hatos-stt`.
- **GPU:** `L4` is a good cheap/fast default; bump to `A10G`/`A100` for heavier models.

## Test it

```bash
curl -X POST -F "audio=@sample.wav" https://<you>--hatos-stt-web.modal.run/transcribe
# -> {"text":"...","ms":380}
```

## Notes

- CORS is wide open (`allow_origins=["*"]`) so the browser can call it directly.
  Lock this down to your app's origin before anything public.
- Audio arrives as whatever the browser's `MediaRecorder` produces (usually
  webm/opus); `ffmpeg` in the image lets faster-whisper decode it.
- The model is cached in a Modal Volume, so re-deploys don't re-download it.
