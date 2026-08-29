#include <time.h>
#include "anim.h"
#include "matrix.h"
#include "state.h"

// today's temperature as an xy plot: local midnight to midnight across the 32
// columns, the day's lo-hi range across the 16 rows. Coloured by temperature
// rather than the fg/bg settings; hi over lo in the top-left, each in its own
// temperature's colour, and a white pixel on the curve marks now. The sky
// above the line is dark at night and light in the day, blended across the
// two columns either side of sunrise and sunset.

// piecewise-linear over anchors every 10F from 40 to 100, clamped outside
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
  float p = (f - 40.0f) / 10.0f;
  if (p <= 0) return RAMP[0];
  if (p >= 6) return RAMP[6];
  int i = (int)p;
  return blend(RAMP[i], RAMP[i + 1], (uint8_t)((p - i) * 255));
}

struct WeatherAnim : Animation {
  void frame(uint32_t) override {
    clear();

    if (!g_weather_hourly_ok) {
      writeString("--", 2, 13, 6, CRGB(90, 90, 90));
      return;
    }

    int lo = g_weather_hourly[0], hi = lo;
    for (int i = 1; i < 24; i++) {
      int t = g_weather_hourly[i];
      if (t < lo) lo = t;
      if (t > hi) hi = t;
    }
    int span = hi > lo ? hi - lo : 1;

    time_t t = time(nullptr);
    struct tm now_tm;
    localtime_r(&t, &now_tm);
    int now_x = (now_tm.tm_hour * 60 + now_tm.tm_min) * (WIDTH - 1) / (23 * 60);
    if (now_x > WIDTH - 1) now_x = WIDTH - 1;

    const CRGB NIGHT(1, 2, 16), DAY(24, 40, 90);
    const float colw = 23.0f * 60 / (WIDTH - 1);  // minutes per column

    int now_y = 0;
    for (int x = 0; x < WIDTH; x++) {
      float h = x * 23.0f / (WIDTH - 1);
      int i = (int)h;
      float temp = g_weather_hourly[i];
      if (i < 23) temp += (g_weather_hourly[i + 1] - g_weather_hourly[i]) * (h - i);
      // animation y=0 is the bottom; 1px margin keeps the line off both edges
      int y = 1 + lroundf((HEIGHT - 3) * (temp - lo) / span);
      CRGB c = tempColor(temp);
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

    setPixel(now_x, now_y, CRGB::White);  // now

    char buf[8];
    writeString(buf, snprintf(buf, sizeof(buf), "%d", hi), 0, 13, tempColor(hi));
    writeString(buf, snprintf(buf, sizeof(buf), "%d", lo), 0, 9, tempColor(lo));
  }
};

static WeatherAnim s_anim;
Animation* const anim_weather = &s_anim;
