#pragma once
#include <stdint.h>

struct Animation {
  virtual void begin() {}
  virtual void frame(uint32_t now_ms) = 0;
};

extern Animation* const anim_clock;
extern Animation* const anim_ca;
extern Animation* const anim_plasma;
extern Animation* const anim_test;
extern Animation* const anim_scroll;
extern Animation* const anim_weather;
extern Animation* const anim_humidity;
