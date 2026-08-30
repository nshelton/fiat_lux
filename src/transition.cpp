#include "transition.h"
#include "matrix.h"

// push between modes: each row is a 64px strip of [old frame | new frame]
// sliding left at its own speed, so the old scene exits the left edge with
// the new one riding in from the right behind it -- the old wipe's motion
// with no pass through black. Rows land one by one as their offset reaches
// WIDTH. The corruption rides the motion -- serpentine smears, latched
// sparkles, chunk glitches -- strongest at the switch and healing to nothing
// as the fade runs out. Sparkles latch at the victim pixel's own brightness
// and smears shift dark onto dark invisibly, so the chaos lives where the
// content is and dark stays dark.

#define T_FADE 15

static CRGB buf[HEIGHT][WIDTH];
static uint8_t speed[HEIGHT];  // px per frame, 3-6
static int frame = -1;         // fade counter, -1 = idle

static uint8_t maxc(const CRGB& c) {
  uint8_t m = c.r > c.g ? c.r : c.g;
  return m > c.b ? m : c.b;
}

void transitionStart() {
  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) buf[y][x] = getPixel(x, y);
    speed[y] = 3 + random8(4);
  }
  frame = 0;
}

void transitionCancel() { frame = -1; }

// display-aligned violence: an 8x4 grid of 4x4 cells, each with a small
// chance per frame of losing its mind. One roll in ten is pure noise, one is
// a solid primary, the rest bit-mash whatever is already there.
static void chunkGlitch() {
  const int CELL = 4;
  for (int cy = 0; cy < HEIGHT; cy += CELL)
    for (int cx = 0; cx < WIDTH; cx += CELL) {
      if (random8() >= 16) continue;
      uint8_t roll = random8(10);
      uint8_t mash = random8(5);
      uint8_t bits = 0x20 << random8(3);
      CRGB solid(0, 0, 0);
      solid.raw[random8(3)] = 255;
      for (int y = cy; y < cy + CELL; y++)
        for (int x = cx; x < cx + CELL; x++) {
          CRGB c = getPixel(x, y);
          if (roll == 0) c = CRGB(random8(), random8(), random8());
          else if (roll == 1) c = solid;
          else switch (mash) {
            case 0: c = CRGB(c.g, c.b, c.r); break;             // channel rotate
            case 1: c.raw[random8(3)] ^= bits; break;           // flip a high bit
            case 2:                                             // xor moire
              c.r ^= ((x & 15) << 4) ^ (y << 4);
              c.g ^= ((x ^ y) & 15) << 4;
              break;
            case 3: c.r <<= 1; c.g <<= 1; c.b <<= 1; break;     // overdrive, wraps
            case 4: c.r &= 0xC0; c.g &= 0xC0; c.b &= 0xC0; break;  // posterize crush
          }
          setPixel(x, y, c);
        }
    }
}

// a corrupt bit shifts everything downstream of it, and leds[] is chain
// order, so the smear zigzags along the serpentine like a starved panel.
static void corrupt(uint8_t smear_chance, uint8_t sparkles) {
  for (int s = 0; s < 2; s++) {
    if (random8() >= smear_chance) continue;
    int len = 8 + random8(32);
    int shift = 1 + random8(3);
    int start = random16(NUM_LEDS - len - shift);
    memmove(&leds[start], &leds[start + shift], len * sizeof(CRGB));
  }
  for (int n = random8(sparkles); n > 0; n--) {
    int i = random16(NUM_LEDS);
    leds[i] = CHSV(random8(), 255, maxc(leds[i]));
  }
  chunkGlitch();
}

// runs after the new mode has drawn its frame; no-op once settled
void transitionPost() {
  if (frame < 0) return;
  if (frame >= T_FADE) {
    frame = -1;
    return;
  }

  // screen pixel x reads strip position x+off: the snapshot while that lands
  // inside it, the new render displaced right past it. x runs high to low so
  // each new-content read happens before its source pixel is overwritten
  for (int y = 0; y < HEIGHT; y++) {
    int off = speed[y] * frame;
    if (off >= WIDTH) continue;  // row landed, leave the new render alone
    for (int x = WIDTH - 1; x >= 0; x--) {
      int sx = x + off;
      setPixel(x, y, sx < WIDTH ? buf[y][sx] : getPixel(sx - WIDTH, y));
    }
  }

  int left = T_FADE - frame;
  corrupt(40 + left * 12, 4 + left);

  frame++;
}
