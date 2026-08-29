#include "transition.h"
#include "matrix.h"

// glitch wipe between modes: the old frame's rows slide off the left edge at
// random speeds while a few grey blocks flash up and decay, ending in black.
// ~12 frames at FRAME_MS = about 400ms.

#define T_FRAMES 12

static CRGB buf[HEIGHT][WIDTH];
static uint8_t speed[HEIGHT];  // px per frame, 3-6
static int frame = -1;         // -1 = idle

struct Block {
  uint8_t x, y, w, h, start;
};
static Block blocks[5];

void transitionStart() {
  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) buf[y][x] = getPixel(x, y);
    speed[y] = 3 + random8(4);
  }
  for (Block& b : blocks) {
    b.w = 3 + random8(6);
    b.h = 2 + random8(4);
    b.x = random8(WIDTH - b.w);
    b.y = random8(HEIGHT - b.h);
    b.start = random8(4);
  }
  frame = 0;
}

bool transitionActive() { return frame >= 0; }
void transitionCancel() { frame = -1; }

bool transitionFrame() {
  if (frame < 0) return false;
  if (frame >= T_FRAMES) {
    frame = -1;
    return false;
  }

  for (int y = 0; y < HEIGHT; y++) {
    int off = speed[y] * frame;
    for (int x = 0; x < WIDTH; x++)
      setPixel(x, y, x + off < WIDTH ? buf[y][x + off] : CRGB::Black);
  }

  // each block ramps up then back down over 8 frames
  for (Block& b : blocks) {
    int t = frame - b.start;
    if (t < 0 || t >= 8) continue;
    uint8_t v = (t < 4 ? t + 1 : 8 - t) * 50;
    for (int y = b.y; y < b.y + b.h; y++)
      for (int x = b.x; x < b.x + b.w; x++) setPixel(x, y, CRGB(v, v, v));
  }

  frame++;
  return true;
}
