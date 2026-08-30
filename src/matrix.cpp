#include "matrix.h"
#include "font.h"

CRGB leds[NUM_LEDS];

void matrixSetup() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_POWER_MA);
}

// FastLED's own power model -- the same math the MAX_POWER_MA limiter
// throttles against, so the clamp here mirrors what show() actually does
uint32_t matrixPowerMa() {
  uint32_t mw = calculate_unscaled_power_mW(leds, NUM_LEDS) * FastLED.getBrightness() / 256;
  uint32_t ma = mw / 5;
  return ma > MAX_POWER_MA ? MAX_POWER_MA : ma;
}

// serpentine within each 16-wide panel, odd rows reversed, panel 2 offset +256
static int XY(int x, int y) {
  int panel = x / PANEL_WIDTH;
  int px = x % PANEL_WIDTH;
  if (y & 1) px = PANEL_WIDTH - 1 - px;
  return panel * (PANEL_WIDTH * HEIGHT) + y * PANEL_WIDTH + px;
}

void setPixel(int x, int y, CRGB col) {
  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
  leds[XY(x, y)] = col;
}

CRGB getPixel(int x, int y) {
  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return CRGB::Black;
  return leds[XY(x, y)];
}

// Stream senders address pixels as a raster index with y=0 at the top of their
// image, which lands at the opposite end of the panel from where the animations
// put y=0 -- streamed frames came out vertically mirrored against the clock. The
// flip lives here so the udp and http paths cannot drift apart.
void setRaster(int idx, CRGB col) {
  setPixel(idx % WIDTH, HEIGHT - 1 - idx / WIDTH, col);
}

void clear(CRGB col) {
  fill_solid(leds, NUM_LEDS, col);
}

void putChar(uint8_t c, int x, int y, uint8_t scale, CRGB col) {
  if (scale == 1) {
    uint16_t bits = glyph3x3(c);
    for (int dy = 0; dy < 3; dy++)
      for (int dx = 2; dx >= 0; dx--) {
        if (bits & 1) setPixel(x + dx, y + dy, col);
        bits >>= 1;
      }
  } else {
    // 5x7 glyphs drawn double-height: the clock face fills the panel with them
    uint64_t bits = glyph5x7(c);
    for (int dy = 0; dy < 7; dy++)
      for (int dx = 5; dx >= 1; dx--) {
        if (bits & 1) {
          setPixel(x + dx, y + 2 * dy, col);
          setPixel(x + dx, y + 2 * dy + 1, col);
        }
        bits >>= 1;
      }
  }
}

void writeString(const char* str, int len, int x, int y, CRGB col, uint8_t scale) {
  int w = (scale == 1) ? 4 : 6;
  for (int i = 0; i < len; i++)
    putChar(str[i], i * w + x, y, scale, col);
}
