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
outdoor and indoor readings across the top, mode buttons, colour pickers, and
the brightness and fader sliders.

The readings live on the page rather than the panel: four numbers squeezed onto
one 32px row were never legible, and the clock has the space to itself now. The
page repolls `/state` every 10s and renders every field in the readout, but
never assigns into the control state — that would yank a slider or picker back
mid-drag. `-1` shows as `--`, which is what weather and the sensor report
before their first successful read.

Two endpoints back the page, and both are just as usable from curl:

- `GET /state` → `{"mode":N,"v":[bri,f1],"fg":"rrggbb","bg":"rrggbb","t":[...],"heap":N,"ma":N,"aqi":N}`
  (`heap` is the free heap in bytes, read live — a number drifting down over
  days is a leak. `ma` is the current frame's LED draw from FastLED's power
  model — the same math the `MAX_POWER_MA` limiter throttles against, so it
  clamps at 3500. A model, not a measurement. `aqi` is the current outdoor US
  AQI, `-1` before the first fetch.)
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
`5` weather, `6` humidity, `7` aqi, `8` world map.

```
curl "http://fiatlux.local/set?mode=4&bri=120"
```

Test patterns (mode 3): the fader picks index chase / column sweep / row sweep /
solid white ramp for verifying panel wiring and the power cap.

The ticker (mode 4) scrolls time, date and both temp/humidity pairs; the fader
sets the speed. It rebuilds the string each wrap so it stays current.

Weather (mode 5) plots today's hourly forecast from open-meteo as an xy graph:
local midnight to midnight across the 32 columns, the day's lo-hi range across
the 16 rows, with the hi stacked over the lo in the top-left, the current
reading top-right, and a full-height grey column marking now — white where it
crosses the curve. The sky above the line is black at night and dim
white in the day, fading across the two columns either side of sunrise and
sunset. The curve is coloured by temperature — 50F blue, 60 green, 70 yellow,
80 orange, 90 red, darkening past 100 — blended between anchors, so the day
sweeps through the scale. Shows `--` until the first fetch lands.

Humidity (mode 6) is the same graph for relative humidity, coloured dry amber
through green and teal to blue at saturation.

AQI (mode 7) is the same graph again for the US AQI, coloured by the standard
bands — green at 0 through yellow 50, orange 100, red 150, purple 200, maroon
300. It comes from open-meteo's air-quality host
(`air-quality-api.open-meteo.com`, same no-key deal, path with lat/lon in
`secrets.h`) on the weather cadence, offset 30s so the loop never stalls for
two fetches at once. Modelled from CAMS satellite data, not a station reading —
citywide, so it can miss the neighbour's barbecue.

The world map (mode 8) is the equirectangular earth at 11.25° per pixel — green
land, blue ocean — with the night side dimmed along the real solar terminator
and a yellow dot at the point where the sun is overhead. It is computed from
UTC alone (declination from the day of year, subsolar longitude from the time),
so it needs no fetch and is never stale; the terminator crawls across at one
column per 45 minutes, and the lit fraction of each pole tracks the seasons.
Until NTP syncs the whole map shows daylit.

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

### over HTTP

For senders without a UDP socket there is an HTTP equivalent:

- `POST /frame` — body is exactly `32 * 16 * 3` = 1536 raw RGB bytes in raster
  order, same layout the UDP path uses. No Content-Length parsing: the body is
  always a full frame, so the handler reads that many bytes or drops it. Replies
  `204`. Same override and 1s-fallback behaviour as UDP.

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
- [x] frame streaming — UDP from `tools/stream.py`, HTTP via `POST /frame`
- [x] one foreground / one background colour, persisted to NVS
- [x] temp + humidity readouts moved off the panel onto the control page
- [x] scrolling text animation
