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

## control (HTTP, port 80)

Open `http://<device-ip>/` — the device serves its own control page with mode
buttons, master brightness and the four animation faders. Two endpoints back it,
and both are just as usable from curl:

- `GET /state` → `{"mode":N,"v":[bri,f1,f2,f3,f4]}`
- `GET /set?mode=N&bri=N&f1=N&f2=N&f3=N&f4=N` → the same JSON. Every param is
  optional and clamped to 0-255, so you can send just the one you care about.

Modes: `0` clock, `1` wolfram CA, `2` plasma, `3` test patterns, `4` ticker.

```
curl "http://192.168.0.222/set?mode=4&bri=120"
```

Test patterns (mode 3): fader1 picks index chase / column sweep / row sweep /
solid white ramp for verifying panel wiring and the power cap.

The ticker (mode 4) scrolls time, date and both temp/humidity pairs; fader1 sets
the speed. It rebuilds the string each wrap so it stays current.

## streaming (UDP, port 8001)

`tools/stream.py <ip>` pushes live frames and runs a plasma demo:

```
python3 tools/stream.py 192.168.0.222
```

Packets are `'F' 'L' seq idx_hi idx_lo count flags` + `count` RGB triplets.
`idx` is a raster pixel index (`y * 32 + x`) — the sender never sees the
serpentine wiring — and `flags` bit 0 closes a frame, which is what triggers the
show. Streaming overrides the selected animation; it resumes 1s after the last
packet.

The header carries its own `count` because WiFiNINA's `parsePacket()` reports
every byte the NINA has queued for the socket rather than one datagram: frames
arrive concatenated and can split across reads, so the receiver walks whole
packets and carries the remainder. It must also never bail with bytes still
queued — the next `parsePacket()` discards them one at a time while more arrive,
which starves the render loop and the HTTP server until the panel looks hung.

## known quirks

- **Pixel glitches at low brightness**: the Nano's 3.3V data is marginal against
  the WS2812 threshold (0.7 x VDD). A heavily loaded supply sags enough to make
  it clean; a dim display lets the rail float high and sparkles return. Dim it
  far enough and the first pixel stops hearing the data entirely: the panel
  latches on its last frame while the Nano keeps running (web page still
  answers). `MIN_BRIGHTNESS` in `src/config.h` floors the slider above that
  point; set it to 0 once the 74AHCT125 level shifter (see roadmap) is in.
- If the board hangs at boot, the serial breadcrumbs (`boot` / `matrix ok` /
  `net ok` / `sensor ok`) show which subsystem is stuck.
- A flaky micro-USB cable produces disappearing/flapping ports and failed
  SAM-BA uploads. If the port won't come back, double-tap reset (amber LED
  pulses = bootloader) and reflash over USB.

## roadmap

- [ ] 74AHCT125 level shifter between pin 2 and panel DIN (fixes glitches)
- [ ] inline ~5A fuse on the DC input
- [x] UDP frame streaming + `tools/stream.py` for live animation prototyping
- [x] scrolling text animation
