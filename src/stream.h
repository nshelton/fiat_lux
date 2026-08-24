#pragma once
#include <stdint.h>

// live pixel streaming: udp from tools/stream.py, http from the control page
bool streamUpdate(uint32_t now, bool net_up);  // true when a whole frame landed
bool streamActive(uint32_t now);               // stream is driving the panel
void streamHttpFrame(uint32_t now);            // POST /frame filled the buffer
