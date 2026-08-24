#pragma once

// ---- matrix: two 16x16 serpentine panels chained left->right = 32x16 ----
#define LED_PIN      2
#define WIDTH        32
#define HEIGHT       16
#define PANEL_WIDTH  16
#define NUM_LEDS     (WIDTH * HEIGHT)
#define LED_TYPE     WS2812
#define COLOR_ORDER  GRB

// 5V 4A supply -- never raise this cap (usb_safe env overrides it down to 300)
#ifndef MAX_POWER_MA
#define MAX_POWER_MA 3500
#endif

#define FRAME_MS     33

// Floor for the brightness slider. Pin 2 drives 3.3V into a 5V panel; dim it
// enough and the rail floats high, the first pixel stops hearing the data and
// the panel latches on its last frame while the Nano keeps running. Tune to
// just above where sparkles start. 0 = off -- set once the 74AHCT125 is in.
#define MIN_BRIGHTNESS 64

// ---- network ----
#define HOSTNAME         "fiatlux"
#define HTTP_PORT        80
#define HTTP_TIMEOUT_MS  200
// US Pacific, DST applied automatically (2nd Sunday Mar - 1st Sunday Nov, 2am).
// For a zone without DST, set both offsets equal.
#define TZ_STD_OFFSET    (-8)
#define TZ_DST_OFFSET    (-7)
#define NTP_RESYNC_MS    (10UL * 60 * 1000)
#define WEATHER_MS       (10UL * 60 * 1000)
#define SENSOR_MS        (60UL * 1000)

// ---- UDP pixel streaming (tools/stream.py) ----
#define STREAM_PORT        8001
#define STREAM_TIMEOUT_MS  1000   // no packets for this long -> local animations resume
#define STREAM_HEADER      7
#define STREAM_BUF         1500   // matches WiFiNINA's per-socket buffer
