#include "anim.h"
#include "matrix.h"

struct PlasmaAnim : Animation {
  void frame(uint32_t now) override {
    uint32_t t = now / 8;
    for (int y = 0; y < HEIGHT; y++)
      for (int x = 0; x < WIDTH; x++) {
        uint8_t v = (sin8(x * 16 + t) + sin8(y * 24 - t) + sin8((x + y) * 8 + t / 2)) / 3;
        setPixel(x, y, CHSV(v + t / 16, 240, 255));
      }
  }
};

static PlasmaAnim s_anim;
Animation* const anim_plasma = &s_anim;
