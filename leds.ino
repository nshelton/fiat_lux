
#include <Adafruit_NeoPixel.h>
#include <chrono>
#include <iostream>

#define LED_PIN 2
#define NUM_LEDS 512
#define WIDTH 32
#define HEIGHT 16

#define CHARWIDTH 4

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_RGB + NEO_KHZ800);

uint32_t g_time = 0;
uint8_t g_ledBrightness = 200;

uint32_t leds[NUM_LEDS];

void setPixel(int x, int y, uint32_t col) {

  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
    return;
  }

  uint32_t idx;
  bool odd_row = y & 1;

  if (x < 16) {
    idx = y * 16 + x;
    if (odd_row) { x = 15 - x; }
    idx = y * 16 + x;
    leds[idx] = col;

  } else if (x >= 16) {
    x -= 16;
    if (odd_row) { x = 15 - x; }
    idx = y * 16 + x + 256;
    leds[idx] = col;
  }
}

void clear(uint32_t color = 0) {
  for (int i = 0; i < NUM_LEDS; i++) { leds[i] = color; }
}

void show() {
  for (int i = 0; i < NUM_LEDS; i++) { strip.setPixelColor(i, leds[i]); }
  strip.show();
}

void putChar(char n, uint32_t x, uint32_t y, uint8_t scale = 1, uint32_t col = 0xffffff) {
  uint32_t C = 0b000010000;

  if (auto search = charmap33.find(n); search != charmap33.end())
    C = search->second;

  for (int dy = 0; dy < 3; dy++) {
    for (int dx = 2; dx >= 0; dx--) {
      if (C & 1) {

        for (int ddx = 0; ddx < scale; ddx++) {
          for (int ddy = 0; ddy < scale; ddy++) {
            setPixel(x + dx * scale + ddx, y + dy * scale + ddy, col);
          }
        }
      }
      C >>= 1;
    }
  }
}

void writeString(char* str, uint32_t len, int x, int y, uint32_t color, int scale = 1) {
  for (uint32_t i = 0; i < len; i++) {
    putChar(str[i], i * CHARWIDTH * scale + x - (scale-1) * i, y, scale, color);
  }
}

void bootSequence(uint32_t time) {
  // clear( strip.Color(128,128,128));
  clear();
  float progress = WIDTH * (float)time / 100.0;

  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < progress; x++) {
      setPixel(x, y, strip.Color(128, 255, 128));
    }
  }
  show();  // Update strip with new contents
}

char timebuffer[14];

uint32_t last_time_printed = 0;

void updateLED() {
  g_time += 10;

  bool DO_CA = false;
  bool DO_BACKGROUND = false;
  
  if (g_time < 1000) {
    bool DO_CA = true;
  }

  // clear(strip.Color(10, 10, 10));
  clear(0);
  

  if (DO_CA) {
    next_ca();
    for (int y = 0; y < HEIGHT; y++) {
      for (int x = 0; x < WIDTH; x++) {
        uint32_t col = strip.ColorHSV(x * 512 + g_time + y * 512, 255, 30);
        if (get_ca(x, y) > 0) {
          setPixel(x, y, col);
        }
      }
    }
  }

  if (DO_BACKGROUND) {
    for (int y = 0; y < HEIGHT; y++) {
      for (int x = 0; x < WIDTH; x++) {
        uint32_t col = strip.ColorHSV(x * 512 + g_time + y * 512, 255, 255);
        setPixel(x, y, col);
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

  uint32_t hue = 64 * 512 + g_time + 8 * 512;
  uint16_t spread = g_fader_1 * 1000;
  writeString(timebuffer+0, 2, 0, 5,  strip.ColorHSV(hue, 255, 255), 2);
  writeString(timebuffer+2, 2, 14, 5, strip.ColorHSV(hue + spread * 1, 255, 255), 2);
  // writeString(":", 1, 13, 5,          strip.ColorHSV(hue, 255, 255), 2);
  writeString(timebuffer+4, 1, 28, 8, strip.ColorHSV(hue + spread * 2, 255, 255), 1);
  writeString(timebuffer+5, 1, 28, 4, strip.ColorHSV(hue + spread * 2, 255, 255), 1);

  writeString(timebuffer+6, 3, 0, 12,   strip.ColorHSV(hue , 255, 255), 1);
  writeString(timebuffer+9, 2, 12, 12,  strip.ColorHSV(hue+spread , 255, 255), 1);
  writeString(timebuffer+11, 3, 20, 12, strip.ColorHSV(hue+spread * 2 , 255, 255), 1);

  // writeString(timeString,  0, 4, strip.Color(0, 255, 0), 2);
  // writeString("34",  16, 0, strip.Color(0, 255, 128), 2);
  // writeString("asdfghjk",  0, 8, strip.Color(0, 0, 255));
  // writeString("12345678", 8, 0, 12, strip.Color(128, 255, 0));

  show();  
}


void setLEDBrightness(uint8_t val) {
  g_ledBrightness = val;
  strip.setBrightness(g_ledBrightness);
}

void setupLED() {

  strip.begin();
  strip.show();
  strip.setBrightness(g_ledBrightness);
  init_ca();
  g_time = 0;
}