// #include "FastLED.h"
// FASTLED_USING_NAMESPACE
// using namespace FastLED

#include <chrono>
#include <iostream>

#define LED_PIN 2
#define NUM_LEDS 512
#define WIDTH 32
#define HEIGHT 16

#define CHARWIDTH1 4
#define CHARWIDTH2 6

uint8_t g_ledBrightness = 200;

CRGB leds[NUM_LEDS];

#define PALETTE_SIZE 8
CRGB g_current_palette[PALETTE_SIZE];

void setPixel(int x, int y, CRGB col) {

  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
    return;
  }

  uint32_t idx;
  bool odd_row = y & 1;

  if (x < 16) {
    idx = y * 16 + x;
    if (odd_row) { x = 15 - x; }
    idx = y * 16 + x;
    // leds[idx] = col / (255 / g_ledBrightness);
    leds[idx] = col;

  } else if (x >= 16) {
    x -= 16;
    if (odd_row) { x = 15 - x; }
    idx = y * 16 + x + 256;
    // leds[idx] = col / (255 / g_ledBrightness);
    leds[idx] = col;
  }
}

void clear(CRGB color = CRGB::BlanchedAlmond) {
  for (int i = 0; i < NUM_LEDS; i++) { leds[i] = color; }
}

void putChar(char n, uint32_t x, uint32_t y, uint8_t scale, CRGB col = CRGB::Wheat) {

  if (scale == 1) {
    uint32_t C = 1;
    if (auto search = charmap33.find(n); search != charmap33.end())
      C = search->second;

    for (int dy = 0; dy < 3; dy++) {
      for (int dx = 2; dx >= 0; dx--) {
        if (C & 1)
          setPixel(x + dx, y + dy, col);
        C >>= 1;
      }
    }
  }

  if (scale == 2) {
    uint64_t C = 1;
    if (auto search = charmap57.find(n); search != charmap57.end())
      C = search->second;

    for (int dy = 0; dy < 7; dy++) {
      for (int dx = 5; dx >= 1; dx--) {
        if (C & 1)
          setPixel(x + dx, y + dy, col);
        C >>= 1;
      }
    }
  }
}

CRGB getColorFromTemp(int temp) {
  if (temp < 0) { return CRGB(0, 0, 10); }
  if (temp < 50) { return CRGB(0, 0, 255); }
  if (temp < 60) { return CRGB(82, 170, 180); }
  if (temp < 70) { return CRGB(100, 150, 91); }
  if (temp < 80) { return CRGB(237, 237, 234); }
  if (temp < 90) { return CRGB(252, 208, 89); }
  return CRGB(222, 88, 66);
}

void writeString(char* str, uint32_t len, int x, int y, CRGB color, int scale = 1) {
  int w = (scale == 1) ? CHARWIDTH1 : CHARWIDTH2;
  for (uint32_t i = 0; i < len; i++) {
    putChar(str[i], i * w + x, y, scale, color);
  }
}

char timebuffer[14];

uint32_t last_time_printed = 0;
uint32_t g_time = 0;

void updateLED() {
  g_time = millis() / 100;
  updatePalette(g_time);

  bool DO_CA = false;
  bool DO_BACKGROUND = false;

  // if (g_time < 1000) {
  //   DO_CA = true;
  // }

  clear(0);
  clear(g_current_palette[PALETTE_SIZE - 1] * 0.2);

  if (DO_CA) {
    next_ca();
    for (int y = 0; y < HEIGHT; y++) {
      for (int x = 0; x < WIDTH; x++) {
        CRGB col = CHSV(x * 512 + g_time + y * 512, 255, 30);
        if (get_ca(x, y) > 0) {
          setPixel(x, y, col / 10);
        }
      }
    }
  }

  if (DO_BACKGROUND) {
    for (int y = 0; y < HEIGHT; y++) {
      for (int x = 0; x < WIDTH; x++) {
        CRGB col = CHSV(x * 512 + g_time + y * 512, 255, 255);
        setPixel(x, y, col / 4);
      }
    }
  }

  // for (int y = 0; y < 3; y++) {
  //   for (int x = 0; x < WIDTH; x++) {
  //       setPixel(x, y, strip.Color(100,100,100));
  //   }
  // }

  getDateTimeString(timebuffer);

  if (g_time - last_time_printed > 1000) {
    Serial.println(timebuffer);
    last_time_printed = g_time;
  }

  writeString(timebuffer + 0, 2, 0, 4, g_current_palette[1], 2);
  writeString(timebuffer + 2, 2, 14, 4, g_current_palette[3], 2);
  writeString(":", 1, 10, 4, g_current_palette[2], 2);

  writeString(timebuffer + 4, 1, 28, 8, g_current_palette[4], 1);
  writeString(timebuffer + 5, 1, 28, 4, g_current_palette[4], 1);

  writeString(timebuffer + 6, 3, 0, 12, g_current_palette[1], 1);
  writeString(timebuffer + 9, 2, 12, 12, g_current_palette[2], 1);
  writeString(timebuffer + 11, 3, 20, 12, g_current_palette[3], 1);

  sprintf(timebuffer, "%d^", g_local_temperature);
  writeString(timebuffer, 3, 0, 0, getColorFromTemp(g_local_temperature), 1);

  sprintf(timebuffer, "%dx", g_local_humidity);
  writeString(timebuffer, 3, 12, 0, g_current_palette[0], 1);

  // writeString(timeString,  0, 4, strip.Color(0, 255, 0), 2);
  // writeString("34",  16, 0, strip.Color(0, 255, 128), 2);
  // writeString("asdfghjk",  0, 8, strip.Color(0, 0, 255));
  // writeString("12345678", 8, 0, 12, strip.Color(128, 255, 0));
  setPixel(0, 0, CRGB(255, 255, 255));
  setPixel(WIDTH, 0, CRGB(255, 255, 255));
  setPixel(0, HEIGHT, CRGB(255, 255, 255));
  setPixel(WIDTH, HEIGHT, CRGB(255, 255, 255));

  FastLED.show();
}

void setLEDBrightness(uint8_t val) {
  g_ledBrightness = val;
  Serial.print("brightness set to ");
  Serial.println(g_ledBrightness);
  // FastLED.setBrightness(g_ledBrightness);
}

void updatePalette(int t) {
  for (int i = 0; i < PALETTE_SIZE; i++) {
    g_current_palette[i] = CHSV(i * 8 + t / 32, 200, g_ledBrightness);
  }
}

void ledBootScreen() {
  for (uint8_t y = 0; y < HEIGHT - 1; y++) {
    for (uint8_t x = 0; x < WIDTH - 1; x++) {
      CRGB col = CHSV(x, y, 64);
      setPixel(x, y, col);
    }
  }

  writeString("connect", 8, 1, 8, CHSV(0, 255, 255), 1);

  FastLED.show();
}


#define LED_TYPE WS2812
#define COLOR_ORDER GRB

void setupLED() {

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  FastLED.setBrightness(250);

  ledBootScreen();

  updatePalette(g_time);

  init_ca();
  g_time = 0;
}