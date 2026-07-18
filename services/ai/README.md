# HatOS AI service

This optional Modal endpoint keeps cloud credentials off the phone and ESP32. It
uses Linkup for current web answers and returns display-sized text.

## Deploy

```bash
python -m pip install -r services/ai/requirements.txt
modal setup
modal secret create linkup-secret LINKUP_API_KEY=your-key
modal deploy services/ai/app.py
```

Put the resulting `modal.run` URL in the companion app only after the offline demo
is stable. Do not commit credentials or embed them in Genesis firmware.

## Request

```json
{ "question": "What is the weather in London today?" }
```

The response includes compact wearer/outward strings plus the sourced answer. The
client should time out quickly and fall back to a scripted response during demos.
