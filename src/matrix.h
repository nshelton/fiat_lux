#pragma once
#include <FastLED.h>
#include "config.h"

extern CRGB leds[NUM_LEDS];

#define PALETTE_SIZE 8
extern CRGB g_palette[PALETTE_SIZE];

void matrixSetup();
void setPixel(int x, int y, CRGB col);
void setRaster(int idx, CRGB col);  // raster index from a stream sender, y flipped
void clear(CRGB col = CRGB::Black);
void putChar(uint8_t c, int x, int y, uint8_t scale, CRGB col);
void writeString(const char* str, int len, int x, int y, CRGB col, uint8_t scale = 1);
void updatePalette(uint32_t t);
