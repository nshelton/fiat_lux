#include "anim.h"
#include "matrix.h"
#include "state.h"
#include "timesync.h"

// status ticker: clock, date, outdoor then indoor readings.
// /1/fader1 sets the speed. Rebuilt each time it wraps, so it stays current.
struct ScrollAnim : Animation {
  char text[96];
  int len = 0;
  int pos = 0;  // pixels scrolled, 1/16ths

  const char* name() override { return "scroll"; }

  void build() {
    char dt[16];
    getDateTimeString(dt);  // hhmmsswwwddmmm
    len = snprintf(text, sizeof(text),
                   "%.2s:%.2s  %.3s %.2s %.3s   OUT %dF %d%%   IN %dF %d%%   ",
                   dt, dt + 2, dt + 6, dt + 9, dt + 11,
                   g_weather_temp, g_weather_humidity, g_sensor_temp, g_sensor_humidity);
  }

  void begin() override {
    pos = 0;
    build();
  }

  void frame(uint32_t now) override {
    updatePalette(now / 100);
    pos += 4 + g_fader[0] / 8;  // 0.25 - 2.2 px per frame

    int x = WIDTH - (pos >> 4);
    if (x <= -len * 4) {
      pos = 0;
      build();
      x = WIDTH;
    }

    clear();
    for (int i = 0; i < len; i++)
      putChar(text[i], x + i * 4, 6, 1, g_palette[i % PALETTE_SIZE]);
  }
};

static ScrollAnim s_anim;
Animation* const anim_scroll = &s_anim;
