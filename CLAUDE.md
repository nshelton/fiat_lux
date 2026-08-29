# fiat lux

32x16 WS2812 LED clock on an Adafruit ItsyBitsy ESP32. See README.md for the
user-facing side; this file is the stuff that bites when you forget it.

## hardware facts

- **512 LEDs, 32 wide x 16 tall** — two 16x16 serpentine panels chained
  left->right. Not 64x32. `XY()` in `matrix.cpp` owns the serpentine reversal
  and the `+256` second-panel offset.
- **Data is on D5 = GPIO5.** That is the only pin on this board with a level
  shifter, so it is not interchangeable with another free GPIO. GPIO2 is the
  onboard NeoPixel's power rail — do not use it.
- **D5 shifts up to `VHi`**, the board's dual-Schottky OR of BAT and USB. `VHi`
  is an *output*; never feed power into it. The panel's 5V goes to **BAT**.
- 5V 4A supply. `MAX_POWER_MA` stays at 3500.
- I2C is SDA=GPIO15, SCL=GPIO27, from the board variant's `SDA`/`SCL`.

## the loop

`main.cpp` is a flat cooperative loop — no tasks, no RTOS primitives. Every
subsystem exposes an `xUpdate(now, ...)` that must return promptly. Nothing may
block: HTTP, mDNS and **OTA are all serviced from this loop**, so anything that
wedges it takes OTA down with it and recovery becomes physical. There is no
watchdog.

Streaming pre-empts animations: `streamActive()` is true for
`STREAM_TIMEOUT_MS` after the last completed frame, and while it holds, the
animation dispatch is skipped entirely.

## orientation

Senders address pixels as a raster index with y=0 at the top of *their* image.
That is the opposite end of the panel from the animations' y=0, so
`setRaster()` in `matrix.cpp` flips y. Both the UDP and HTTP paths go through
it — do not reimplement the index math at a call site, that is exactly the drift
it exists to prevent.

## colours

One foreground and one background, `0xRRGGBB` in `state.h` (packed rather than
`CRGB` so `state.h` stays free of FastLED). Used by clock, ticker and the
wolfram CA. Plasma, weather and the test patterns keep their own — plasma is a
colour field with no foreground in it, weather is coloured by temperature, and
the test pattern's red pixel 0 is how you find the start of the chain.

Persisted to NVS, but **never write NVS on every update**: the colour picker
sends one per mouse move and flash has finite erase cycles. `prefsUpdate()`
waits for `PREFS_SETTLE_MS` of no change.

## envs

```
pio run -e itsybitsy -t upload   # USB
pio run -e ota       -t upload   # wifi, espota:3232
```

- `panel` — matrix + test patterns, **no wifi linked in**. The point is
  isolation: a glitch here cannot be wifi contention, so it is the 5V data path
  or FastLED's RMT timing. Reach for this before debugging display problems.
- `bringup` — blink + serial heartbeat, for board/cable/USB only.

`panel` and `bringup` have no networking, so they cannot be flashed over OTA.

## diagnosing a dead panel

Two failures look identical from across the room and need opposite fixes:

```
curl http://fiatlux.local/state
```

Answers → the MCU is fine, the panel has gone deaf (hardware). Silent, but
`ping` still replies → `loop()` is wedged, needs a power cycle. Silent and no
ping → it is off or off-network.

## gotchas

- `param()` clamps to 0-255; `paramHex()` takes exactly six hex digits or
  returns -1. Colours travel without a `#` — a `#` in a URL starts a fragment
  and never reaches the device.
- The control page repolls `/state` and renders every field in the readout,
  but **never assigns into `s`** — doing so would yank a slider or picker back
  mid-drag.
- FastLED + wifi on ESP32 can flicker via RMT contention. If it appears,
  `-DFASTLED_RMT_MAX_CHANNELS=1` or the I2S driver, not the data path.
