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

  // supply sag kills blue first, then green: whites go amber, then ember
  uint8_t sag = 255 - frame * 20;
  uint8_t bsag = scale8(sag, sag);

  for (int y = 0; y < HEIGHT; y++) {
    int off = speed[y] * frame;
    for (int x = 0; x < WIDTH; x++) {
      CRGB c = x + off < WIDTH ? buf[y][x + off] : CRGB::Black;
      c.g = scale8(c.g, sag);
      c.b = scale8(c.b, bsag);
      setPixel(x, y, c);
    }
  }

  // each block ramps up then back down over 8 frames, sagging like the rest
  for (Block& b : blocks) {
    int t = frame - b.start;
    if (t < 0 || t >= 8) continue;
    uint8_t v = (t < 4 ? t + 1 : 8 - t) * 50;
    CRGB c(v, scale8(v, sag), scale8(v, bsag));
    for (int y = b.y; y < b.y + b.h; y++)
      for (int x = b.x; x < b.x + b.w; x++) setPixel(x, y, c);
  }

  // a corrupt bit shifts everything downstream of it, and leds[] is chain
  // order, so the smear zigzags along the serpentine like a starved panel
  if (random8() < 60 + frame * 12) {
    int len = 8 + random8(32);
    int shift = 1 + random8(3);
    int start = random16(NUM_LEDS - len - shift);
    memmove(&leds[start], &leds[start + shift], len * sizeof(CRGB));
  }

  // pixels latching one frame of garbage, more as the brownout deepens
  for (int n = random8(2 + frame); n > 0; n--)
    leds[random16(NUM_LEDS)] = CHSV(random8(), 255, 255);

  frame++;
  return true;
}
