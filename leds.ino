
// #include <Adafruit_NeoPixel.h>


// #include "FastLED.h"
// FASTLED_USING_NAMESPACE
// using namespace FastLED

#include <chrono>
#include <iostream>

#define LED_PIN 2
#define NUM_LEDS 512
#define WIDTH 32
#define HEIGHT 16

#define CHARWIDTH 4

uint32_t g_time = 0;
uint8_t g_ledBrightness = 200;

CRGB leds[NUM_LEDS];

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
    leds[idx] = col;

  } else if (x >= 16) {
    x -= 16;
    if (odd_row) { x = 15 - x; }
    idx = y * 16 + x + 256;
    leds[idx] = col;
  }
}

void clear(CRGB color = CRGB::BlanchedAlmond) {
  for (int i = 0; i < NUM_LEDS; i++) { leds[i] = color; }
}

void putChar(char n, uint32_t x, uint32_t y, uint8_t scale = 1, CRGB col = CRGB::Wheat) {
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

void writeString(char* str, uint32_t len, int x, int y, CRGB color, int scale = 1) {
  for (uint32_t i = 0; i < len; i++) {
    putChar(str[i], i * CHARWIDTH * scale + x - (scale-1) * i, y, scale, color);
  }
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
        CRGB col = CHSV(x * 512 + g_time + y * 512, 255, 30);
        if (get_ca(x, y) > 0) {
          setPixel(x, y, col);
        }
      }
    }
  }

  if (DO_BACKGROUND) {
    for (int y = 0; y < HEIGHT; y++) {
      for (int x = 0; x < WIDTH; x++) {
        CRGB col = CHSV(x * 512 + g_time + y * 512, 255, 255);
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

  uint32_t hue = g_time ;
  uint16_t spread = g_fader_1 * 10;
  writeString(timebuffer+0, 2, 0, 5,  CHSV(hue, 255, 255), 2);
  writeString(timebuffer+2, 2, 14, 5, CHSV(hue + spread * 1, 255, 255), 2);
  // writeString(":", 1, 13, 5,          CHSV(hue, 255, 255), 2);
  writeString(timebuffer+4, 1, 28, 8, CHSV(hue + spread * 2, 255, 255), 1);
  writeString(timebuffer+5, 1, 28, 4, CHSV(hue + spread * 2, 255, 255), 1);

  writeString(timebuffer+6, 3, 0, 12,   CHSV(hue , 255, 255), 1);
  writeString(timebuffer+9, 2, 12, 12,  CHSV(hue+spread , 255, 255), 1);
  writeString(timebuffer+11, 3, 20, 12, CHSV(hue+spread * 2 , 255, 255), 1);

  // writeString(timeString,  0, 4, strip.Color(0, 255, 0), 2);
  // writeString("34",  16, 0, strip.Color(0, 255, 128), 2);
  // writeString("asdfghjk",  0, 8, strip.Color(0, 0, 255));
  // writeString("12345678", 8, 0, 12, strip.Color(128, 255, 0));
  // send the 'leds' array out to the actual LED strip
  // insert a delay to keep the framerate modest
  // FastLED.delay(100); 

  FastLED.show();  
}


void setLEDBrightness(uint8_t val) {
  g_ledBrightness = val;
  // strip.setBrightness(g_ledBrightness);
}

#define LED_TYPE    WS2812
#define COLOR_ORDER GRB 

void ledBootScreen() {

    for (uint8_t y = 0; y < HEIGHT; y++) {
      for (uint8_t x = 0; x < WIDTH; x++) {
        CRGB col = CHSV(x, y, 64);
        setPixel(x, y, col);
      }
    }

  writeString("connect", 8, 1, 8, CHSV(0, 255, 255), 1);
  FastLED.show();  
}



void setupLED() {

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,LED_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  FastLED.setBrightness(255);

  ledBootScreen();
  // strip.begin();
  // strip.show();
  // strip.setBrightness(g_ledBrightness);
  init_ca();
  g_time = 0;
}