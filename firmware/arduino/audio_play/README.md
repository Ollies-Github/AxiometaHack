# audio_play

Streams a 16-bit PCM clip to the MAX98357A (Port 2) over I²S.

The audio itself is **not committed** — supply your own file. The clip is embedded
into flash via `audio_data.S` (`.incbin`), which avoids a huge C array / slow
compile.

## Generate the clip

```bash
# any source file -> mono, 16 kHz, 16-bit PCM, cleaned up for a small speaker
ffmpeg -ss 15 -t 30 -i your_audio.mp3 -ac 1 -ar 16000 \
  -af "highpass=f=150,loudnorm=I=-13:TP=-1.5" -f s16le clip.raw
```

Place `clip.raw` next to the sketch and point `audio_data.S`'s `.incbin` at its
**absolute path** (the Arduino build copies sources to a temp dir, so a relative
path won't resolve). ~30 s at 16 kHz mono ≈ 940 KB.

## Build

Needs the big-app partition for the embedded audio:

```
FQBN=esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=4M,PSRAM=enabled,PartitionScheme=huge_app
```

Only use audio you have the rights to. Do not commit copyrighted audio.
