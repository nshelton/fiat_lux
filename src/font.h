#pragma once
#include <stdint.h>

uint16_t glyph3x3(uint8_t c);  // 9 bits, rows of 3
uint64_t glyph5x7(uint8_t c);  // 35 bits, rows of 5; digits and ':' only
