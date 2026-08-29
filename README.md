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

Neither `panel` nor `bringup` links in wifi, so neither can be flashed over OTA.

Other envs:
- `panel` — matrix and test patterns only, no wifi linked in. Walks the index
  chase up a brightness ladder (1, 2, 4 ... 255), announcing each step on
  serial. Use it to validate the data path in isolation: with no wifi in the
  build, a glitch here is the 5V line or FastLED's RMT timing and nothing else.
- `bringup` — blink + serial heartbeat only, for debugging board/cable/USB.

## control (HTTP, port 80)

Open `http://fiatlux.local/` — the device serves its own control page with the
outdoor and indoor readings across the top, mode buttons, colour pickers,
streaming, and the brightness and fader sliders.

The readings live on the page rather than the panel: four numbers squeezed onto
one 32px row were never legible, and the clock has the space to itself now. The
page repolls `/state` every 10s and applies only `t` from it — taking the whole
object would yank a slider or picker back mid-drag. `-1` shows as `--`, which is
what weather and the sensor report before their first successful read.

Two endpoints back the page, and both are just as usable from curl:

- `GET /state` → `{"mode":N,"v":[bri,f1],"fg":"rrggbb","bg":"rrggbb","t":[...]}`
- `GET /set?mode=N&bri=N&f1=N&fg=rrggbb&bg=rrggbb` → the same JSON. Every param
  is optional, the numeric ones clamp to 0-255, so you can send just the one you
  care about.

`f1` is the only fader. It means something different per mode: in mode 3 it
picks the test pattern, in mode 4 it sets the ticker speed. There used to be an
`f2`-`f4` alongside it as free params for animations; nothing ever read them.

Colours are six hex digits with no `#` — a `#` in a URL starts a fragment and
never reaches the device. A value that is not exactly six hex digits is ignored
rather than half-parsed.

```
curl "http://fiatlux.local/set?fg=ff8800&bg=001020"
```

The two pickers set one foreground and one background, used by the clock, the
ticker and the wolfram CA (live cells are `fg`, the rest `bg`). Plasma, the
weather plot and the test patterns keep their own colours: plasma *is* a colour
field with no foreground in it, weather is coloured by temperature, and the
test pattern's red pixel 0 is how you find the start of the chain.

They persist to NVS. The picker streams a new colour on every mouse move and NVS
has finite erase cycles, so `prefsUpdate()` only writes once a colour has held
still for `PREFS_SETTLE_MS` — drag freely, one write lands when you settle, and
`colors saved` appears on serial. `prefsLoad()` runs before `matrixSetup()`, so
the first frame after a reboot is already the right colour.

Modes: `0` clock, `1` wolfram CA, `2` plasma, `3` test patterns, `4` ticker,
`5` weather.

```
curl "http://fiatlux.local/set?mode=4&bri=120"
```

Test patterns (mode 3): the fader picks index chase / column sweep / row sweep /
solid white ramp for verifying panel wiring and the power cap.

The ticker (mode 4) scrolls time, date and both temp/humidity pairs; the fader
sets the speed. It rebuilds the string each wrap so it stays current.

Weather (mode 5) plots today's hourly forecast from open-meteo as an xy graph:
local midnight to midnight across the 32 columns, the day's lo-hi range across
the 16 rows, with the hi stacked over the lo in the top-left and a short grey
bar mid-screen marking now. The sky above the line is dark blue at night and
light in the day, fading across the two columns either side of sunrise and
sunset. The curve is coloured by temperature — 50F blue, 60 green, 70 yellow,
80 orange, 90 red, darkening past 100 — blended between anchors, so the day
sweeps through the scale. Shows `--` until the first fetch lands.

## streaming (UDP, port 8001)

`tools/stream.py` pushes live frames and runs a plasma demo:

```
python3 tools/stream.py fiatlux.local
```

Packets are `'F' 'L' seq idx_hi idx_lo count flags` + `count` RGB triplets.
`idx` is a raster pixel index (`y * 32 + x`) with `y = 0` at the top of the
image — the sender never sees the serpentine wiring or the panel's orientation,
both of which `setRaster()` in `src/matrix.cpp` handles. `flags` bit 0 closes a
frame, which is what triggers the show.

Streaming overrides the selected animation; it resumes 1s after the last packet.

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
full tilt; brightness applies too, so push the slider to 255 first.

The canvas below the buttons is a live preview of what is being sent, so the
page is useful without line of sight to the panel.

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
- HTTP, mDNS and OTA are all serviced from `loop()`, so anything that blocks it
  takes OTA with it and recovery is a power cycle. There is no watchdog. To tell
  a wedged loop from a deaf panel: `curl /state` answering means the MCU is fine
  and the panel is not listening; silence while `ping` still replies means the
  loop is stuck.
- The AHT reads the panel as much as the room if it is mounted near the matrix —
  512 LEDs put out real heat. Move it away for a meaningful indoor number.

## roadmap

- [ ] inline ~5A fuse on the DC input
- [ ] watchdog — a wedged `loop()` currently needs a physical power cycle,
      because OTA is serviced from that same loop
- [ ] move the AHT off the panel so the indoor reading means the room
- [x] ItsyBitsy ESP32 port — D5's level shifter fixes the low-brightness
      glitches the Nano's 3.3V data caused; the brightness floor that worked
      around them is gone and the slider runs to 0
- [x] frame streaming — UDP from `tools/stream.py`, HTTP from the control page
- [x] one foreground / one background colour, persisted to NVS
- [x] temp + humidity readouts moved off the panel onto the control page
- [x] scrolling text animation
