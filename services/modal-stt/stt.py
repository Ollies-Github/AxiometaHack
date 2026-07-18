"""HatOS voice — fast Whisper speech-to-text on Modal.

A single warm L4 GPU container runs faster-whisper behind a CORS-enabled FastAPI
app, so the phone/browser can POST mic audio straight to it (no bridge in the
middle). Deploy, grab the printed URL, paste it into the app's Voice screen.

    pip install modal
    modal setup                       # one-time auth (opens browser)
    modal deploy stt.py               # prints https://<you>--hatos-stt-web.modal.run

Then POST multipart audio (field "audio") to  <that-url>/transcribe
  ->  {"text": "...", "ms": 412}

Model is overridable:  HATOS_STT_MODEL=large-v3-turbo modal deploy stt.py
(good options, fast→accurate: distil-small.en, distil-large-v3, large-v3-turbo)
"""
import io
import os
import time

import modal

MODEL = os.environ.get("HATOS_STT_MODEL", "distil-large-v3")
CACHE_DIR = "/cache"
MINUTES = 60

image = (
    # CUDA runtime image — ctranslate2 (faster-whisper's backend) needs
    # libcublas/cudnn 12 on the GPU, which debian_slim doesn't ship.
    modal.Image.from_registry("nvidia/cuda:12.4.1-cudnn-runtime-ubuntu22.04", add_python="3.11")
    .apt_install("ffmpeg")  # lets PyAV/faster-whisper decode webm/opus from the browser
    .pip_install("faster-whisper", "fastapi[standard]", "numpy")
)

app = modal.App("hatos-stt")
cache = modal.Volume.from_name("hatos-stt-cache", create_if_missing=True)


@app.function(
    image=image,
    gpu="L4",
    min_containers=1,             # keep one container warm for low latency
    scaledown_window=10 * MINUTES,
    timeout=10 * MINUTES,
    volumes={CACHE_DIR: cache},   # persist the downloaded model across deploys
)
@modal.concurrent(max_inputs=8)
@modal.asgi_app()
def web():
    import numpy as np
    from fastapi import FastAPI, File, UploadFile
    from fastapi.middleware.cors import CORSMiddleware
    from faster_whisper import WhisperModel

    print(f"loading faster-whisper '{MODEL}' on GPU...")
    model = WhisperModel(MODEL, device="cuda", compute_type="float16", download_root=CACHE_DIR)
    cache.commit()
    # Warm the CUDA kernels on silence so the first real request is fast.
    list(model.transcribe(np.zeros(16000, dtype=np.float32), language="en")[0])
    print("model ready")

    api = FastAPI()
    api.add_middleware(
        CORSMiddleware, allow_origins=["*"], allow_methods=["*"], allow_headers=["*"]
    )

    @api.get("/")
    def health():
        return {"ok": True, "model": MODEL}

    @api.post("/transcribe")
    async def transcribe(audio: UploadFile = File(...)):
        t0 = time.time()
        data = await audio.read()
        segments, info = model.transcribe(
            io.BytesIO(data), language="en", beam_size=1
        )
        text = " ".join(seg.text.strip() for seg in segments).strip()
        return {"text": text, "ms": int((time.time() - t0) * 1000)}

    return api
