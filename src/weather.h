#pragma once
#include <stdint.h>

void weatherUpdate(uint32_t now, bool net_up);
void aqiUpdate(uint32_t now, bool net_up);
