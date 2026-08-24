#pragma once

// ---- matrix: two 16x16 serpentine panels chained left->right = 32x16 ----
// Data goes out on D5 (GPIO5), the ItsyBitsy's level-shifted 5V output pin --
// output only, that is what it is for. Feed the panel supply into the board's
// USB pin so the shifter and the WS2812s share one rail: the 0.7*VDD input
// threshold then tracks the drive level instead of racing it.
#define LED_PIN      5
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

// Floor for the brightness slider. 0 because D5 drives real 5V logic; the
// Nano's 3.3V data sat under the WS2812 threshold and needed 64 to stay clean.
#ifndef MIN_BRIGHTNESS
#define MIN_BRIGHTNESS 0
#endif

// ---- network ----
#define HOSTNAME         "fiatlux"
#define HTTP_PORT        80
#define HTTP_TIMEOUT_MS  200
// POSIX TZ, US Pacific. The esp32 sntp client applies the DST rule itself.
// For a zone without DST, drop everything after the offset ("PST8").
#define TZ_POSIX         "PST8PDT,M3.2.0,M11.1.0"
#define NTP_SERVER       "pool.ntp.org"
#define WEATHER_MS       (10UL * 60 * 1000)
#define SENSOR_MS        (60UL * 1000)

// ---- UDP pixel streaming (tools/stream.py) ----
#define STREAM_PORT        8001
#define STREAM_TIMEOUT_MS  1000   // no packets for this long -> local animations resume
#define STREAM_HEADER      7
// count is one byte, so this is the largest a single packet can be
#define STREAM_BUF         (STREAM_HEADER + 255 * 3)
