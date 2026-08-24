# fiat lux

LED clock: 32x16 WS2812 matrix (two 16x16 serpentine panels chained left->right),
driven by an Adafruit ItsyBitsy ESP32, AHT temp/humidity sensor on I2C.
Power is a 5V 4A supply — the FastLED power cap in `src/config.h` must stay at 3500 mA.

## wiring

Panel data comes off **D5 (GPIO5)**, the ItsyBitsy's level-shifted 5V output —
output only, which is exactly what it is there for. Put ~330R in series at the
board end and keep a ground wire paired with the data run.

Power the board off the panel's 5V through the **BAT** pin (rated 3.5-6V in).
BAT and USB are diode-OR'd on board into `VHi`, and `VHi` is what D5 shifts up
to -- so the shifter's high side is ~5V less a Schottky drop either way, and the
laptop's USB cable can stay plugged in for serial while the panel supply is
live. `VHi` is that OR gate's *output*: do not feed power into it.

Because BAT is diode-isolated from `VHi`, USB power cannot flow back out to the
panel. On USB alone the panel just stays dark instead of browning out the port.

Never run the panel's own supply current through the board. The panel takes its
several amps on its own leads straight from the PSU; the board only taps ~250 mA
off that rail for itself.

Panel ground and board ground still have to meet at the panel's DIN end — the
shifter's output swings relative to the board's ground, so that is the
reference the first pixel measures against.

## build & flash

```
cp include/secrets.example.h include/secrets.h   # fill in wifi creds + your lat/lon
pio run -e itsybitsy -t upload                   # over USB (only needed the first time)
pio run -e ota -t upload                         # over wifi (espota, port 3232)
```

The device prints its IP on serial (115200 baud) at boot and advertises itself
as `fiatlux.local`. The OTA password in `secrets.h` must match `--auth` in
`[env:ota]`; set `upload_port` to the IP if mDNS does not resolve for you.

Other envs:
- `usb_safe` — LED power capped at 300 mA. Left over from the Nano, where the
  panel back-fed off the board's 5V rail and brownout-looped the USB port. With
  the panel on BAT the diode-OR blocks that path, so this is belt-and-braces.
- `panel` — matrix and test patterns only, no wifi linked in. Walks the index
  chase up a brightness ladder (1, 2, 4 ... 255), announcing each step on
  serial. Use it to validate the data path in isolation: with no wifi in the
  build, a glitch here is the 5V line or FastLED's RMT timing and nothing else.
- `bringup` — blink + serial heartbeat only, for debugging board/cable/USB.

## control (HTTP, port 80)

Open `http://fiatlux.local/` — the device serves its own control page with mode
buttons, master brightness and the four animation faders. Two endpoints back it,
and both are just as usable from curl:

- `GET /state` → `{"mode":N,"v":[bri,f1,f2,f3,f4]}`
- `GET /set?mode=N&bri=N&f1=N&f2=N&f3=N&f4=N` → the same JSON. Every param is
  optional and clamped to 0-255, so you can send just the one you care about.

Modes: `0` clock, `1` wolfram CA, `2` plasma, `3` test patterns, `4` ticker.

```
curl "http://fiatlux.local/set?mode=4&bri=120"
```

Test patterns (mode 3): fader1 picks index chase / column sweep / row sweep /
solid white ramp for verifying panel wiring and the power cap.

The ticker (mode 4) scrolls time, date and both temp/humidity pairs; fader1 sets
the speed. It rebuilds the string each wrap so it stays current.

## streaming (UDP, port 8001)

`tools/stream.py` pushes live frames and runs a plasma demo:

```
python3 tools/stream.py fiatlux.local
```

Packets are `'F' 'L' seq idx_hi idx_lo count flags` + `count` RGB triplets.
`idx` is a raster pixel index (`y * 32 + x`) with `y = 0` at the top of the
image — the sender never sees the serpentine wiring or the panel's orientation,
both of which `setRaster()` in `src/matrix.cpp` handles. `flags` bit 0 closes a
frame, which is what triggers the show. Streaming overrides the selected animation; it resumes 1s after the last
packet.

Many frames can land between render passes; the receiver drains all of them and
shows only the last, so a fast sender drops stale frames instead of queueing
them.

### from the browser

Browsers cannot open a UDP socket, so the control page renders patterns itself
and POSTs whole frames instead:

- `POST /frame` — body is exactly `32 * 16 * 3` = 1536 raw RGB bytes in raster
  order, same layout the UDP path uses. No Content-Length parsing: the body is
  always a full frame, so the handler reads that many bytes or drops it. Replies
  `204`.

The `stream` row on the page picks a pattern; `off` stops sending and the panel
falls back to its animation after the usual 1s. `white` and `black` are the
stress-test pair -- both are *held* (the pump keeps posting), so strobe between
those two rather than between `white` and `off`. Note `white` is still subject
to `MAX_POWER_MA`, so it measures the FastLED limiter's ceiling, not 512 LEDs at
full tilt; brightness applies too, so push the slider to 255 first. The canvas above the faders is a
live preview of what is being sent, so the page is useful without line of sight
to the panel.

Adding a pattern there is a function and a `PAT` entry in `src/http.cpp`, same
contract as `tools/stream.py` — fill `f[(y * W + x) * 3]` with RGB. State
between frames is fine (`fire` keeps a heat buffer; `bounce` scales the previous
frame down instead of clearing, which is where its trail comes from).

Awaiting each POST is the backpressure — never more than one frame in flight, so
a slow link degrades the frame rate instead of queueing up latency. A connection
per frame sounds wasteful but measures ~34 fps on a quiet LAN, comfortably above
the 30 fps the UDP sender targets, so it was not worth keeping connections alive
or adding a websocket library.

## known quirks

- If the board hangs at boot, the serial breadcrumbs (`boot` / `matrix ok` /
  `net ok` / `sensor ok`) show which subsystem is stuck.
- ESP32 resets when its own rail sags and WiFi transmit pulls ~500 mA peaks, so
  a marginal supply shows up as a brownout reset rather than a glitchy panel.

## roadmap

- [ ] inline ~5A fuse on the DC input
- [x] ItsyBitsy ESP32 port — D5's level shifter fixes the low-brightness
      glitches the Nano's 3.3V data caused; the brightness floor that worked
      around them is gone and the slider runs to 0
- [x] UDP frame streaming + `tools/stream.py` for live animation prototyping
- [x] scrolling text animation
