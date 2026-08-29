#include "transition.h"
#include "matrix.h"

// glitch wipe between modes, two phases. Out: the old frame's rows slide off
// the left edge at random speeds while grey blocks flash and the panel
// "browns out" -- blue dies first, then green, smears follow the serpentine,
// pixels latch garbage. In: the new mode renders normally and the same
// corruption is applied on top of it, heavy at first, healing to clean --
// the scene condenses out of the noise, still drifting leftward into place.

#define T_OUT 12
#define T_IN  10

static CRGB buf[HEIGHT][WIDTH];
static uint8_t speed[HEIGHT];  // px per frame, 3-6
static int frame = -1;         // out-phase counter, -1 = idle
static int in = 0;             // in-phase intensity, counts down to 0

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
  in = 0;
}

bool transitionActive() { return frame >= 0; }

void transitionCancel() {
  frame = -1;
  in = 0;
}

// a corrupt bit shifts everything downstream of it, and leds[] is chain
// order, so the smear zigzags along the serpentine like a starved panel
static void corrupt(uint8_t smear_chance, uint8_t sparkles) {
  if (random8() < smear_chance) {
    int len = 8 + random8(32);
    int shift = 1 + random8(3);
    int start = random16(NUM_LEDS - len - shift);
    memmove(&leds[start], &leds[start + shift], len * sizeof(CRGB));
  }
  for (int n = random8(sparkles); n > 0; n--)
    leds[random16(NUM_LEDS)] = CHSV(random8(), 255, 255);
}

bool transitionFrame() {
  if (frame < 0) return false;
  if (frame >= T_OUT) {
    frame = -1;
    in = T_IN;
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

  corrupt(60 + frame * 12, 2 + frame);

  frame++;
  return true;
}

// runs after the new mode has drawn its frame; no-op once settled
void transitionPost() {
  if (in <= 0) return;

  uint8_t sag = 255 - in * 20;
  uint8_t bsag = scale8(sag, sag);

  // rows arrive displaced right and settle leftward into place, the same
  // direction everything left by
  for (int y = 0; y < HEIGHT; y++) {
    int off = speed[y] * in / 2;
    for (int x = WIDTH - 1; x >= 0; x--) {
      CRGB c = x - off >= 0 ? getPixel(x - off, y) : CRGB::Black;
      c.g = scale8(c.g, sag);
      c.b = scale8(c.b, bsag);
      setPixel(x, y, c);
    }
  }

  corrupt(40 + in * 15, 1 + in);

  in--;
}
