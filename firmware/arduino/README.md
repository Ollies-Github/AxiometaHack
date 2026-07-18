# Arduino sketches (Genesis Mini)

Working, hardware-verified sketches. Pin maps are documented in
[`../HARDWARE.md`](../HARDWARE.md). Board FQBN:

```
esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=4M,PSRAM=enabled
```

| Sketch | What it does |
|--------|--------------|
| `encoder_display/` | Rotary encoder (P1) → live gauge on the ST7735 TFT (P4), with servo (P3) + buzzer (P2) demo. Reference integration of all four ports. |
| `audio_play/` | Streams an embedded 16 kHz PCM clip to the MAX98357A amp (P2) over I²S. Encoder = volume, press = pause. Needs `PartitionScheme=huge_app`. |
| `audio_test/` | Synthesized I²S tone/melody test for the MAX98357A — quickest "is the amp alive" check. |
| `displayprobe/` | Brute-forces the ST7735 CS/DC/RST pin order — reusable for bringing up any new SPI display module. |
| `pinscan/` | Prints which GPIOs toggle when you move a module. Fastest way to locate an unknown module's pins. |
| `i2c_scan/` | Scans the shared I²C bus (both SDA/SCL orientations). |
| `discovery/` | Combined display + all-port GPIO watch used during bring-up. |

These diagnostics (`displayprobe`, `pinscan`, `i2c_scan`) are how the pin maps in
`HARDWARE.md` were confirmed — keep them for bringing up future AX22 modules.
