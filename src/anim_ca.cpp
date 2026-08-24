#include "anim.h"
#include "matrix.h"
#include "state.h"

// elementary CA on the top row, history scrolls down
static uint8_t grid[NUM_LEDS];
static const uint8_t RULE[8] = {0, 0, 0, 1, 1, 1, 1, 0};

static uint8_t& cell(int x, int y) {
  return grid[(x + WIDTH) % WIDTH + WIDTH * y];
}

struct CaAnim : Animation {
  uint32_t last_step = 0;

  const char* name() override { return "wolfram"; }

  void begin() override {
    for (int i = 0; i < NUM_LEDS; i++) grid[i] = random(0, 2);
  }

  void frame(uint32_t now) override {
    if (now - last_step > 100) {
      last_step = now;
      for (int y = HEIGHT - 1; y >= 1; y--)
        for (int x = 0; x < WIDTH; x++) cell(x, y) = cell(x, y - 1);
      for (int x = 0; x < WIDTH; x++)
        cell(x, 0) = RULE[cell(x - 1, 1) * 4 + cell(x, 1) * 2 + cell(x + 1, 1)];
    }

    clear(CRGB(g_bg));
    for (int y = 0; y < HEIGHT; y++)
      for (int x = 0; x < WIDTH; x++)
        if (cell(x, y)) setPixel(x, y, CRGB(g_fg));
  }
};

static CaAnim s_anim;
Animation* const anim_ca = &s_anim;
