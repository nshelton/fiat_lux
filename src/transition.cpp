#include "transition.h"
#include "matrix.h"

// glitch wipe between modes, two phases. Out: the old frame's rows slide off
// the left edge at random speeds while the panel "browns out" -- blue dies
// first, green next, red lingers -- and colour blocks (some inverting what is
// under them) flash up, drift left and fade. Smears follow the serpentine,
// pixels latch garbage. In: the new mode renders normally and the same
// corruption is applied on top, healing to clean over ten frames.
//
// All of it feeds on the content actually there: sparkles latch at the target
// pixel's own brightness (black stays black), smears shift dark onto dark
// invisibly, and blocks -- their count scaled by the outgoing frame's energy
// -- spawn centred on lit pixels and take their brightness from them. A full
// white screen dies violently; a lone ticker stripe just crackles around
// itself.

#define T_OUT 12
#define T_IN  10

static CRGB buf[HEIGHT][WIDTH];
static uint8_t speed[HEIGHT];  // px per frame, 3-6
static int frame = -1;         // out-phase counter, -1 = idle
static int in = 0;             // in-phase intensity, counts down to 0

struct Block {
  uint8_t x, y, w, h, start, spd;
  CRGB col;
  bool inv;  // invert what is underneath instead of painting col
};
static Block blocks[9];

static uint8_t maxc(const CRGB& c) {
  uint8_t m = c.r > c.g ? c.r : c.g;
  return m > c.b ? m : c.b;
}

void transitionStart() {
  uint32_t e = 0;
  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      buf[y][x] = getPixel(x, y);
      e += maxc(buf[y][x]);
    }
    speed[y] = 3 + random8(4);
  }
  uint8_t energy = e / NUM_LEDS;

  int nb = 2 + energy * 8 / 256;
  for (int i = 0; i < (int)(sizeof(blocks) / sizeof(blocks[0])); i++) {
    Block& b = blocks[i];
    b.start = 255;  // disabled unless a lit spawn point turns up
    if (i >= nb) continue;
    for (int tries = 0; tries < 8; tries++) {
      uint8_t x = random8(WIDTH), y = random8(HEIGHT);
      uint8_t v = maxc(buf[y][x]);
      if (v < 40) continue;
      b.w = 3 + random8(6);
      b.h = 2 + random8(4);
      b.x = x > b.w / 2 ? (x - b.w / 2 + b.w > WIDTH ? WIDTH - b.w : x - b.w / 2) : 0;
      b.y = y > b.h / 2 ? (y - b.h / 2 + b.h > HEIGHT ? HEIGHT - b.h : y - b.h / 2) : 0;
      b.start = random8(4);
      b.spd = 2 + random8(4);
      b.col = CHSV(random8(), 255, v);
      b.inv = random8() < 90;
      break;
    }
  }
  frame = 0;
  in = 0;
}

bool transitionActive() { return frame >= 0; }

void transitionCancel() {
  frame = -1;
  in = 0;
}

// per-channel supply sag: blue's forward voltage gives out first, then
// green, red hangs on to the end
static uint8_t sagLevel(int level, int rate) {
  int v = 255 - level * rate;
  return v < 0 ? 0 : v;
}

static void sagPixel(CRGB& c, uint8_t rs, uint8_t gs, uint8_t bs) {
  c.r = scale8(c.r, rs);
  c.g = scale8(c.g, gs);
  c.b = scale8(c.b, bs);
}

// a corrupt bit shifts everything downstream of it, and leds[] is chain
// order, so the smear zigzags along the serpentine like a starved panel.
// Sparkles latch garbage at the victim pixel's own brightness, so the
// chaos lives where the content is and dark stays dark.
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
}

bool transitionFrame() {
  if (frame < 0) return false;
  if (frame >= T_OUT) {
    frame = -1;
    in = T_IN;
    return false;
  }

  uint8_t rs = sagLevel(frame, 8), gs = sagLevel(frame, 28), bs = sagLevel(frame, 52);

  for (int y = 0; y < HEIGHT; y++) {
    int off = speed[y] * frame;
    for (int x = 0; x < WIDTH; x++) {
      CRGB c = x + off < WIDTH ? buf[y][x + off] : CRGB::Black;
      sagPixel(c, rs, gs, bs);
      setPixel(x, y, c);
    }
  }

  // blocks flash up then down over 8 frames while drifting left with the rest
  for (Block& b : blocks) {
    int t = frame - b.start;
    if (t < 0 || t >= 8) continue;
    uint8_t v = (t < 4 ? t + 1 : 8 - t) * 50;
    int bx = b.x - t * b.spd;
    for (int y = b.y; y < b.y + b.h; y++)
      for (int x = bx; x < bx + b.w; x++) {
        if (b.inv) {
          CRGB c = getPixel(x, y);
          setPixel(x, y, blend(c, CRGB(255 - c.r, 255 - c.g, 255 - c.b), v));
        } else {
          CRGB c = b.col;
          c.nscale8(v);
          setPixel(x, y, c);
        }
      }
  }

  corrupt(90 + frame * 14, 8 + frame * 2);

  frame++;
  return true;
}

// runs after the new mode has drawn its frame; no-op once settled
void transitionPost() {
  if (in <= 0) return;

  uint8_t rs = sagLevel(in, 8), gs = sagLevel(in, 28), bs = sagLevel(in, 52);

  // rows arrive displaced right and settle leftward into place, the same
  // direction everything left by
  for (int y = 0; y < HEIGHT; y++) {
    int off = speed[y] * in / 2;
    for (int x = WIDTH - 1; x >= 0; x--) {
      CRGB c = x - off >= 0 ? getPixel(x - off, y) : CRGB::Black;
      sagPixel(c, rs, gs, bs);
      setPixel(x, y, c);
    }
  }

  corrupt(60 + in * 16, 6 + in * 2);

  in--;
}
