#include "anim.h"
#include "matrix.h"
#include "state.h"

// hardware verification patterns, /1/fader1 selects:
//   0: index chase (led 0 red, walks the physical chain in wiring order)
//   1: column sweep   2: row sweep   3: solid white ramp (power cap check)
struct TestAnim : Animation {
  const char* name() override { return "test"; }

  void frame(uint32_t now) override {
    clear();
    switch (g_fader / 64) {
      case 0:
        leds[0] = CRGB::Red;
        leds[(now / 100) % NUM_LEDS] = CRGB::White;
        break;
      case 1: {
        int x = (now / 200) % WIDTH;
        for (int y = 0; y < HEIGHT; y++) setPixel(x, y, CRGB::White);
        break;
      }
      case 2: {
        int y = (now / 200) % HEIGHT;
        for (int x = 0; x < WIDTH; x++) setPixel(x, y, CRGB::White);
        break;
      }
      case 3: {
        uint8_t v = triwave8(now / 64);
        fill_solid(leds, NUM_LEDS, CRGB(v, v, v));
        break;
      }
    }
  }
};

static TestAnim s_anim;
Animation* const anim_test = &s_anim;
