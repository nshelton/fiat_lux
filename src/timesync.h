#pragma once
#include <stdint.h>

void timeSyncUpdate(uint32_t now, bool net_up);
void getDateTimeString(char* buf);  // 15 bytes: "hhmmsswwwddmmm"
