#include "anim.h"
#include "matrix.h"
#include "state.h"
#include "timesync.h"

struct ClockAnim : Animation {
  const char* name() override { return "clock"; }

  void frame(uint32_t now) override {
    clear(CRGB(g_bg));
    CRGB fg = CRGB(g_fg);

    char buf[16];
    getDateTimeString(buf);  // hhmmsswwwddmmm

    writeString(buf + 0, 2, 0, 4, fg, 2);   // hour
    writeString(":", 1, 10, 4, fg, 2);
    writeString(buf + 2, 2, 14, 4, fg, 2);  // minute
    writeString(buf + 4, 1, 28, 8, fg, 1);  // seconds, stacked
    writeString(buf + 5, 1, 28, 4, fg, 1);

    writeString(buf + 6, 3, 0, 12, fg, 1);   // weekday
    writeString(buf + 9, 2, 12, 12, fg, 1);  // day
    writeString(buf + 11, 3, 20, 12, fg, 1); // month
  }
};

static ClockAnim s_anim;
Animation* const anim_clock = &s_anim;
