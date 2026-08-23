#pragma once
#include <stdint.h>

void netSetup();
bool netUpdate(uint32_t now);  // true while wifi is connected; polls OTA
