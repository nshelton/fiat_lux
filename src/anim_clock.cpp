#include "anim.h"
#include "matrix.h"
#include "state.h"
#include "timesync.h"

static CRGB tempColor(int f) {
  if (f < 0) return CRGB(0, 0, 10);
  if (f < 50) return CRGB(0, 0, 255);
  if (f < 60) return CRGB(82, 170, 180);
  if (f < 70) return CRGB(100, 150, 91);
  if (f < 80) return CRGB(237, 237, 234);
  if (f < 90) return CRGB(252, 208, 89);
  return CRGB(222, 88, 66);
}

struct ClockAnim : Animation {
  const char* name() override { return "clock"; }

  void frame(uint32_t now) override {
    updatePalette(now / 100);
    clear();

    char buf[16];
    getDateTimeString(buf);  // hhmmsswwwddmmm

    writeString(buf + 0, 2, 0, 4, g_palette[1], 2);   // hour
    writeString(":", 1, 10, 4, g_palette[2], 2);
    writeString(buf + 2, 2, 14, 4, g_palette[3], 2);  // minute
    writeString(buf + 4, 1, 28, 8, g_palette[4], 1);  // seconds, stacked
    writeString(buf + 5, 1, 28, 4, g_palette[4], 1);

    writeString(buf + 6, 3, 0, 12, g_palette[1], 1);   // weekday
    writeString(buf + 9, 2, 12, 12, g_palette[2], 1);  // day
    writeString(buf + 11, 3, 20, 12, g_palette[3], 1); // month

    char num[8];
    snprintf(num, sizeof(num), "%d", g_weather_temp);
    writeString(num, strlen(num), 0, 0, tempColor(g_weather_temp), 1);
    snprintf(num, sizeof(num), "%d", g_weather_humidity);
    writeString(num, strlen(num), 8, 0, g_palette[0], 1);
    snprintf(num, sizeof(num), "%d", g_sensor_temp);
    writeString(num, strlen(num), 16, 0, tempColor(g_sensor_temp), 1);
    snprintf(num, sizeof(num), "%d", g_sensor_humidity);
    writeString(num, strlen(num), 24, 0, g_palette[0], 1);
  }
};

static ClockAnim s_anim;
Animation* const anim_clock = &s_anim;
