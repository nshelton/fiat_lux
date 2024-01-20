
#include <Adafruit_NeoPixel.h>
#include <chrono>
#include <iostream>

#define LED_PIN 2
#define NUM_LEDS 512
#define WIDTH 32
#define HEIGHT 16
#define BRIGHTNESS 200

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_RGB + NEO_KHZ800);

uint32_t g_time = 0;
uint8_t g_ledBrightness = 120;

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

void putChar(char n, int x, int y, int scale = 1, int col = 0xffffff) {
  uint32_t C = 511;

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

void writeString(std::string str, int x, int y, uint32_t color, int scale = 1) {
  int i = 0;
  int range =  str.length() * scale;
  if (range > WIDTH) {range = WIDTH;}
  for (int px = x; px < range; px += 4 * scale) {
    putChar(str[i++], px, y, scale, color);
  }
}

void bootSequence(uint32_t time) {
  clear( strip.Color(128,128,128));
  float progress =  WIDTH * (float)time / 100.0;

  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < progress; x++) {
      setPixel(x, y,  strip.Color(128,255,128));
    }
  }
  show();  // Update strip with new contents
}

void updateLED() {
  g_time += 10;

  if (g_time < 100) {
    bootSequence(g_time);
    return;
  }

  clear(strip.Color(10,10,10));

  // for (int y = 6; y < 10; y++) {
  //   for (int x = 0; x < WIDTH; x++) {
  //     uint32_t col = strip.ColorHSV(x * 512 + g_time + y * 512, 255, 255);
  //     setPixel(x, y, col);
  //   }
  // }
  // Chronos::DateTime::setTime(epoch());
  // Chronos::DateTime::now().printTo(Serial);

  writeString("tue",  0, 0, strip.Color(128, 255, 0));
  writeString("jan",  15, 0, strip.Color(0, 255, 0));
  writeString("10",  0, 4, strip.Color(0, 255, 0), 2);
  writeString("34",  16, 4, strip.Color(0, 255, 128), 2);
  

  // writeString("asdfghjk",  0, 8, strip.Color(0, 0, 255));
  writeString("12345678",  0, 12, strip.Color(128, 255, 0));

  show();  // Update strip with new contents
}


void setLEDBrightness(uint8_t val) {
  strip.setBrightness(val);
  g_ledBrightness = val;
}

void setupLED() {

  strip.begin();
  strip.show();
  strip.setBrightness(BRIGHTNESS);

  g_time = 0;
}