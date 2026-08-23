# fiat lux

LED clock: 32x16 WS2812 matrix (two 16x16 serpentine panels chained left->right),
driven by an Arduino Nano 33 IoT on pin 2, AHT temp/humidity sensor on I2C.
Power is a 5V 4A supply — the FastLED power cap in `src/config.h` must stay at 3500 mA.

## build & flash

```
cp include/secrets.example.h include/secrets.h   # fill in wifi creds + your lat/lon
pio run -e nano_33_iot -t upload                 # over USB (only needed the first time)
pio run -e ota -t upload                         # over wifi (set upload_port to device IP)
```

The device prints its IP on serial (115200 baud) at boot. The OTA password in
`secrets.h` must match the `curl -u` password in `[env:ota]`.

Other envs:
- `usb_safe` — LED power capped at 300 mA. Use whenever the wall supply is
  unplugged: the panel back-feeds off the Nano's 5V rail, and at full power it
  browns out the USB port in a reset loop.
- `bringup` — blink + serial heartbeat only, for debugging board/cable/USB.

## control (OSC over UDP, port 8000, TouchOSC "Simple" layout page 1)

- `/1/toggle1-4` — select animation: clock, wolfram CA, plasma, test patterns
- `/1/fader5` — master brightness
- `/1/fader1-4` — free animation params

Test patterns (`toggle4`): fader1 picks index chase / column sweep / row sweep /
solid white ramp for verifying panel wiring and the power cap.

## known quirks

- **Pixel glitches at low brightness**: the Nano's 3.3V data is marginal against
  the WS2812 threshold (0.7 x VDD). A heavily loaded supply sags enough to make
  it clean; a dim display lets the rail float high and sparkles return. Real fix
  is the 74AHCT125 level shifter (see roadmap).
- If the board hangs at boot, the serial breadcrumbs (`boot` / `matrix ok` /
  `net ok` / `sensor ok`) show which subsystem is stuck.
- A flaky micro-USB cable produces disappearing/flapping ports and failed
  SAM-BA uploads. If the port won't come back, double-tap reset (amber LED
  pulses = bootloader) and reflash over USB.

## roadmap

- [ ] 74AHCT125 level shifter between pin 2 and panel DIN (fixes glitches)
- [ ] inline ~5A fuse on the DC input
- [ ] UDP frame streaming + `tools/stream.py` for live animation prototyping
- [ ] scrolling text animation (fonts and writeString already support it)
