#include <time.h>
#include "anim.h"
#include "matrix.h"
#include "state.h"

// today's hourly forecast as an xy plot: local midnight to midnight across the
// 32 columns, the day's lo-hi range across the 16 rows. Weather draws the
// temperature, humidity the relative humidity — same graph, different colour
// ramp. Hi over lo in the top-left, the current reading top-right, each in its
// own value's colour, and a full-height grey column with a white pixel on the
// curve marks now. The sky above
// the line is dark at night and light in the day, blended across the two
// columns either side of sunrise and sunset.

static CRGB rampLerp(const CRGB* r, int n, float p) {
  if (p <= 0) return r[0];
  if (p >= n - 1) return r[n - 1];
  int i = (int)p;
  return blend(r[i], r[i + 1], (uint8_t)((p - i) * 255));
}

// anchors every 10F from 40 to 100, clamped outside
static CRGB tempColor(float f) {
  static const CRGB RAMP[] = {
      CRGB(0, 60, 255),    // 40 deep blue
      CRGB(0, 170, 255),   // 50 blue
      CRGB(0, 210, 60),    // 60 green
      CRGB(230, 210, 0),   // 70 yellow
      CRGB(255, 110, 0),   // 80 orange
      CRGB(255, 0, 0),     // 90 red
      CRGB(150, 0, 20),    // 100 dark red
  };
  return rampLerp(RAMP, 7, (f - 40.0f) / 10.0f);
}

// anchors every 20% from bone dry to saturated
static CRGB humColor(float h) {
  static const CRGB RAMP[] = {
      CRGB(200, 90, 10),   // 0 dry amber
      CRGB(230, 190, 0),   // 20 yellow
      CRGB(60, 210, 40),   // 40 green
      CRGB(0, 200, 150),   // 60 teal
      CRGB(0, 110, 255),   // 80 blue
      CRGB(60, 0, 255),    // 100 violet
  };
  return rampLerp(RAMP, 6, h / 20.0f);
}

static void drawGraph(const int* hourly, int current, CRGB (*color)(float)) {
  clear();

  if (!g_weather_hourly_ok) {
    writeString("--", 2, 13, 6, CRGB(90, 90, 90));
    return;
  }

  int lo = hourly[0], hi = lo;
  for (int i = 1; i < 24; i++) {
    int v = hourly[i];
    if (v < lo) lo = v;
    if (v > hi) hi = v;
  }
  int span = hi > lo ? hi - lo : 1;

  time_t t = time(nullptr);
  struct tm now_tm;
  localtime_r(&t, &now_tm);
  int now_x = (now_tm.tm_hour * 60 + now_tm.tm_min) * (WIDTH - 1) / (23 * 60);
  if (now_x > WIDTH - 1) now_x = WIDTH - 1;

  // below ~10 the colour correction and brightness scaling quantize the
  // channels apart (purple) or to zero entirely
  const CRGB NIGHT(0, 0, 0), DAY(10, 10, 10);
  const float colw = 23.0f * 60 / (WIDTH - 1);  // minutes per column

  int now_y = 0;
  for (int x = 0; x < WIDTH; x++) {
    float h = x * 23.0f / (WIDTH - 1);
    int i = (int)h;
    float v = hourly[i];
    if (i < 23) v += (hourly[i + 1] - hourly[i]) * (h - i);
    // animation y=0 is the bottom; 1px margin keeps the line off both edges
    int y = 1 + lroundf((HEIGHT - 3) * (v - lo) / span);
    CRGB c = color(v);
    CRGB fill = c;
    fill.nscale8(30);
    for (int fy = 0; fy < y; fy++) setPixel(x, fy, fill);
    setPixel(x, y, c);

    CRGB sky = CRGB::Black;
    if (g_sun_rise >= 0 && g_sun_set >= 0) {
      float up = ((h * 60 - g_sun_rise) / colw + 2) / 4;
      float dn = ((g_sun_set - h * 60) / colw + 2) / 4;
      float d = up < dn ? up : dn;
      d = d < 0 ? 0 : (d > 1 ? 1 : d);
      sky = blend(NIGHT, DAY, (uint8_t)(d * 255));
    }
    for (int fy = y + 1; fy < HEIGHT; fy++) setPixel(x, fy, sky);
    if (x == now_x) now_y = y;
  }

  // now: a grey column over everything, white where it crosses the curve
  for (int y = 0; y < HEIGHT; y++) setPixel(now_x, y, CRGB(60, 60, 60));
  setPixel(now_x, now_y, CRGB::White);

  char buf[8];
  writeString(buf, snprintf(buf, sizeof(buf), "%d", hi), 0, 13, color(hi));
  writeString(buf, snprintf(buf, sizeof(buf), "%d", lo), 0, 9, color(lo));
  int n = snprintf(buf, sizeof(buf), "%d", current);
  writeString(buf, n, WIDTH - (n * 4 - 1), 13, color(current));
}

struct WeatherAnim : Animation {
  void frame(uint32_t) override { drawGraph(g_weather_hourly, g_weather_temp, tempColor); }
};

struct HumidityAnim : Animation {
  void frame(uint32_t) override { drawGraph(g_humidity_hourly, g_weather_humidity, humColor); }
};

static WeatherAnim s_anim;
Animation* const anim_weather = &s_anim;
static HumidityAnim s_hum;
Animation* const anim_humidity = &s_hum;
