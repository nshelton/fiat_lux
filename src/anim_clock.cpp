#include "anim.h"
#include "matrix.h"
#include "state.h"
#include "timesync.h"

struct ClockAnim : Animation {
  void frame(uint32_t now) override {
    clear(CRGB(g_bg));
    CRGB fg = CRGB(g_fg);

    char buf[16];
    getDateTimeString(buf);  // hhmmsswwwddmmm

    writeString(buf + 0, 2, 0, 1, fg, 2);   // hour, 14 rows tall
    writeString(":", 1, 10, 1, fg, 2);
    writeString(buf + 2, 2, 14, 1, fg, 2);  // minute

    // seconds: four 15-px bars, one pixel per second, empty again at :00
    int sec = (buf[4] - '0') * 10 + (buf[5] - '0');
    for (int i = 0; i < 4; i++) {
      int h = sec - i * 15;
      if (h > 15) h = 15;
      for (int y = 0; y < h; y++)
        setPixel(28 + i, y, fg);
    }
  }
};

static ClockAnim s_anim;
Animation* const anim_clock = &s_anim;
