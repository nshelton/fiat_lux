#pragma once
#include <stdint.h>

// live pixel streaming from tools/stream.py
bool streamUpdate(uint32_t now, bool net_up);  // true when a whole frame landed
bool streamActive(uint32_t now);               // stream is driving the panel
