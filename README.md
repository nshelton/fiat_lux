# fiat lux

LED clock: 32x16 WS2812 matrix (two 16x16 serpentine panels chained left->right),
driven by an Arduino Nano 33 IoT on pin 2, AHT temp/humidity sensor on I2C.
Power is a 5V 4A supply — the FastLED power cap in `src/config.h` must stay at 3500 mA.

## build

```
cp include/secrets.example.h include/secrets.h   # fill in wifi creds + your lat/lon
pio run -e nano_33_iot -t upload                 # first flash over USB
pio run -e ota -t upload                         # after that, over wifi (set upload_port to device IP)
```

The device prints its IP on serial (115200 baud) once wifi is up.

## control (OSC over UDP, port 8000, TouchOSC "Simple" layout page 1)

- `/1/toggle1-4` — select animation: clock, wolfram CA, plasma, test patterns
- `/1/fader5` — master brightness
- `/1/fader1-4` — free animation params

Test patterns (`toggle4`): fader1 picks index chase / column sweep / row sweep /
solid white ramp for verifying panel wiring and the power cap.
