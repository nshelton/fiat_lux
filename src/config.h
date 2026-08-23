#pragma once

// ---- matrix: two 16x16 serpentine panels chained left->right = 32x16 ----
#define LED_PIN      2
#define WIDTH        32
#define HEIGHT       16
#define PANEL_WIDTH  16
#define NUM_LEDS     (WIDTH * HEIGHT)
#define LED_TYPE     WS2812
#define COLOR_ORDER  GRB

// 5V 4A supply -- never raise this cap
#define MAX_POWER_MA 3500

#define FRAME_MS     33

// ---- network ----
#define HOSTNAME         "fiatlux"
#define OSC_PORT         8000
// US Pacific, DST applied automatically (2nd Sunday Mar - 1st Sunday Nov, 2am).
// For a zone without DST, set both offsets equal.
#define TZ_STD_OFFSET    (-8)
#define TZ_DST_OFFSET    (-7)
#define NTP_RESYNC_MS    (10UL * 60 * 1000)
#define WEATHER_MS       (10UL * 60 * 1000)
#define SENSOR_MS        (60UL * 1000)
