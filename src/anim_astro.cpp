#include <math.h>
#include <time.h>
#include "anim.h"
#include "astro.h"
#include "matrix.h"

// the sky in the tropical zodiac. Top band: the sun as its sign's glyph in
// gold, the moon's sign in silver, the moon disc drawn with the real phase
// shape, and the illuminated % over a small triangle -- tip up waxing, tip
// down waning. Bottom band: the zodiac scrolling on a loop, each sign trailed
// by the planets currently in it, sign and planet sharing the planet's
// colour, a red R behind a retrograde planet. Empty signs stay grey.

// zodiac glyphs, aries..pisces: 6 wide x 7 tall, one byte per row, top row
// first, bit 5 = leftmost pixel. Design them as-viewed; the y flip is here.
static const uint8_t ZODIAC[12][7] = {
    {0b100001, 0b100001, 0b010010, 0b001100, 0b001100, 0b001100, 0b001100},  // aries
    {0b100001, 0b010010, 0b011110, 0b100001, 0b100001, 0b100001, 0b011110},  // taurus
    {0b111111, 0b010010, 0b010010, 0b010010, 0b010010, 0b010010, 0b111111},  // gemini
    {0b011111, 0b110000, 0b110000, 0b000011, 0b000011, 0b111110, 0b000000},  // cancer
    {0b001100, 0b010010, 0b010010, 0b110001, 0b110001, 0b000001, 0b000010},  // leo
    {0b101010, 0b101010, 0b101010, 0b101010, 0b101010, 0b000111, 0b000001},  // virgo
    {0b001100, 0b010010, 0b010010, 0b110011, 0b000000, 0b111111, 0b000000},  // libra
    {0b101010, 0b101010, 0b101010, 0b101010, 0b101010, 0b000010, 0b000111},  // scorpio
    {0b001111, 0b000011, 0b000101, 0b001000, 0b010000, 0b101000, 0b010000},  // sagittarius
    {0b100100, 0b100100, 0b010100, 0b001100, 0b000101, 0b000101, 0b000010},  // capricorn
    {0b010101, 0b101010, 0b000000, 0b010101, 0b101010, 0b000000, 0b000000},  // aquarius
    {0b100001, 0b010010, 0b010010, 0b111111, 0b010010, 0b010010, 0b100001},  // pisces
};

// mercury..saturn, same encoding as the zodiac
static const uint8_t PLANET[5][7] = {
    {0b010010, 0b001100, 0b010010, 0b010010, 0b001100, 0b001100, 0b011110},  // mercury
    {0b001100, 0b010010, 0b010010, 0b001100, 0b001100, 0b011110, 0b001100},  // venus
    {0b001111, 0b000011, 0b000101, 0b011100, 0b100010, 0b100010, 0b011100},  // mars
    {0b011010, 0b100010, 0b010010, 0b111111, 0b000010, 0b000010, 0b000110},  // jupiter
    {0b010000, 0b111000, 0b010000, 0b011100, 0b010010, 0b000010, 0b000100},  // saturn
};
static const CRGB PLANET_COL[5] = {
    CRGB(150, 140, 120), CRGB(230, 200, 120), CRGB(220, 50, 20),
    CRGB(200, 140, 60), CRGB(180, 160, 90)};

static const CRGB SUN(255, 170, 0);
static const CRGB MOON(190, 190, 205), MOON_DARK(10, 10, 16), MOON_DIM(110, 110, 130);
static const CRGB TEXT(140, 140, 140), RETRO(220, 30, 30), EMPTY(60, 60, 70);

static void drawGlyph(const uint8_t* g, int x, int top, CRGB col) {
  for (int dy = 0; dy < 7; dy++)
    for (int dx = 0; dx < 6; dx++)
      if (g[dy] >> (5 - dx) & 1) setPixel(x + dx, top - dy, col);
}

struct AstroAnim : Animation {
  void frame(uint32_t now) override {
    clear();

    time_t t = time(nullptr);
    if (t < 1700000000) {  // no NTP yet, nothing here is knowable
      writeString("--", 2, 13, 7, TEXT);
      return;
    }

    static const float DEG = PI / 180.0f;
    double d = astroDays(t);
    float slon = sunEclipticLon(d);
    float mlon, mlat;
    moonEcliptic(d, &mlon, &mlat);
    float elong = fmodf(mlon - slon + 360.0f, 360.0f);  // 0 new, 180 full
    bool waxing = elong < 180.0f;
    int illum = lroundf((1.0f - cosf(elong * DEG)) * 50.0f);

    drawGlyph(ZODIAC[(int)(slon / 30.0f) % 12], 0, 15, SUN);
    drawGlyph(ZODIAC[(int)(mlon / 30.0f) % 12], 8, 15, MOON_DIM);

    // moon disc, the lit limb by phase -- waxing grows from the right edge,
    // waning shrinks toward the left, as seen looking south
    float ce = cosf(elong * DEG);
    for (int dy = -3; dy <= 3; dy++)
      for (int dx = -3; dx <= 3; dx++) {
        if (dx * dx + dy * dy > 11) continue;
        float w = sqrtf(11.0f - dy * dy);  // half-chord of this row
        bool lit = waxing ? dx >= w * ce - 0.5f : dx <= -w * ce + 0.5f;
        setPixel(18 + dx, 12 + dy, lit ? MOON : MOON_DARK);
      }

    // % right-aligned to the edge, the phase triangle tucked underneath
    char buf[8];
    int n = snprintf(buf, sizeof(buf), "%d", illum);
    writeString(buf, n, 33 - n * 4, 12, TEXT);
    setPixel(29, waxing ? 10 : 9, MOON);
    for (int dx = -1; dx <= 1; dx++) setPixel(29 + dx, waxing ? 9 : 10, MOON);

    // bottom band: build the ribbon, then draw it twice so the loop wraps.
    // A glyph entry is 8px, an R is 5.
    float plon[5];
    bool retro[5];
    for (int p = 0; p < 5; p++) {
      plon[p] = planetEclipticLon(d, p);
      retro[p] = fmodf(planetEclipticLon(d + 1.0, p) - plon[p] + 540.0f, 360.0f) - 180.0f < 0;
    }
    struct Entry { const uint8_t* g; CRGB col; };  // g null = the R
    Entry seq[22];
    int ns = 0, total = 0;
    for (int s = 0; s < 12; s++) {
      CRGB col = EMPTY;
      for (int p = 0; p < 5; p++)
        if ((int)(plon[p] / 30.0f) % 12 == s && col == EMPTY) col = PLANET_COL[p];
      seq[ns++] = {ZODIAC[s], col};
      for (int p = 0; p < 5; p++) {
        if ((int)(plon[p] / 30.0f) % 12 != s) continue;
        seq[ns++] = {PLANET[p], PLANET_COL[p]};
        if (retro[p]) seq[ns++] = {nullptr, RETRO};
      }
    }
    for (int i = 0; i < ns; i++) total += seq[i].g ? 8 : 5;

    int x = -(int)(now / 100 % total);  // 10 px/s leftward
    for (int pass = 0; pass < 2; pass++)
      for (int i = 0; i < ns; i++) {
        int w = seq[i].g ? 8 : 5;
        if (x + w > 0 && x < WIDTH) {
          if (seq[i].g) drawGlyph(seq[i].g, x, 6, seq[i].col);
          else writeString("R", 1, x, 2, seq[i].col);
        }
        x += w;
      }
  }
};

static AstroAnim s_anim;
Animation* const anim_astro = &s_anim;
