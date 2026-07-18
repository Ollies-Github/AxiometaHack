# Genesis Mini — verified hardware map

Empirically confirmed on real hardware (not from product-page examples — those use
a different board's GPIOs and are wrong for the Mini). Every pin below was checked
by flashing the board and observing behaviour over serial / on the display.

Board: **ESP32-S3-Mini-N4R2** (4 MB flash, 2 MB PSRAM), 4× AX22 ports.
Enumerates over the built-in USB-Serial-JTAG as `/dev/cu.usbmodem*`.

## AX22 port → GPIO map

**Shared buses (identical on every port):**

| Signal | GPIO |
|--------|------|
| MOSI   | 12   |
| MISO   | 13   |
| SCK    | 14   |
| SDA    | 10   |
| SCL    | 11   |

**Port-unique pins** (`pin3 = IO0/ADC`, `pin4 = IO1/CS/PWM/Tx`, `pin5 = IO2/Rx`):

| Port | IO0 | IO1 | IO2 |
|------|-----|-----|-----|
| 1    | 4   | 3   | 2   |
| 2    | 7   | 6   | 5   |
| 3    | 9   | 16  | 15  |
| 4    | 1   | 17  | 18  |

Schematic reference designators `U1..U4` map 1:1 to physical ports 1..4 (verified
twice: the encoder on port 1 and the display on port 4 both matched).

## Verified module wiring

### AX22-0003 Rotary Encoder (EC11) — tested on Port 1
- `A / CLK` = IO1, `B / DT` = IO2, `SW` = IO0, common → GND. On-board 10k pull-ups.
- **Button is active-HIGH** (on-board pull-down; idle LOW, pressed HIGH) — *not*
  active-low as you'd assume.
- 4 quadrature edges per detent. Decode with an IRAM ISR; do **not** call a library
  `tick()` from both ISR and loop (races and cancels counts).
- Port 1 → CLK=GPIO3, DT=GPIO2, button=GPIO4.

### AX22-0034 "IPS LCD 0.96"" — tested on Port 4
- It is a **ST7735S 160×80 colour SPI TFT**, *not* an I²C OLED.
- `Adafruit_ST7735` + `initR(INITR_MINI160x80)`, `setRotation(3)`, `invertDisplay(true)`.
- SDA=MOSI(12), SCL=SCK(14). Control pins **do not follow the AX22 "IO1=CS" naming** —
  the panel FPC order is: **CS=IO0, DC=IO2, RST=IO1**.
- Port 4 → CS=GPIO1, DC=GPIO18, RST=GPIO17. Backlight is always on.

### AX22-0053 Audio Amp (MAX98357A) — tested on Port 2 ✅ WORKS
- Real **I²S** class-D amp (not analog / not a buzzer tone pin).
- `DIN = IO0`, `BCLK = IO1`, `LRCLK = IO2`. VDD from the port's 5 V.
- Port 2 → DIN=GPIO7, BCLK=GPIO6, LRCLK=GPIO5.
- Driven with the ESP32-S3 I²S peripheral: 16-bit, stereo frames (duplicate the mono
  sample L/R), `I2S_COMM_FORMAT_STAND_I2S`. Plays synthesized tones **and** streamed
  PCM audio cleanly. On-board jumpers set channel (SD_MODE) and gain (default 9 dB).
- **This supersedes the old plan's "audio only on the phone" limit** — full audio
  runs on the hat.

### AX22-0018 Passive Buzzer (MLT-8530)
- MOSFET gate on IO1, AC-coupled — must be driven with a PWM/square wave (LEDC tone),
  DC does nothing. Superseded by the MAX98357A for real audio.

### AX22-0031 Servo Adapter (3 slots) — tested on Port 3
- Slot signals: Slot1/2/3 = IO0/IO1/IO2. Port 3 → GPIO9 / GPIO16 / GPIO15.
- 50 Hz PWM via LEDC (own timer), 500–2500 µs pulse for 0–180°.
- **Needs external servo power on the adapter's J1 (pin2 = +5–6 V, pin1 = GND)** or a
  JP1 solder-bridge to the 3.3 V rail. The port's Vcc is *not* wired to the servo rail
  by default, so the servo gets a valid signal but no power and won't move.

## Flashing

```bash
FQBN="esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=4M,PSRAM=enabled"
arduino-cli compile -b "$FQBN" <sketch>
arduino-cli upload  -b "$FQBN" -p /dev/cu.usbmodem101 <sketch>
```

- Use `PartitionScheme=huge_app` when embedding audio (3 MB app partition).
- If a sketch crash-loops, the normal uploader can't sync. Flash the built binaries
  directly with esptool's USB-JTAG reset:
  ```bash
  python3 -m esptool --chip esp32s3 --port /dev/cu.usbmodem101 \
    --before usb_reset --after hard_reset write_flash -z \
    0x0 <bootloader>.bin 0x8000 <partitions>.bin 0x10000 <app>.bin
  ```
- Read serial at 115200; `s.setDTR(False); s.setRTS(False)` in pyserial to avoid resets.
