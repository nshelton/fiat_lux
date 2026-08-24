#pragma once
#include <stdint.h>

void prefsLoad();                 // restore saved colours, call before the first frame
void prefsUpdate(uint32_t now);   // writes them back once they settle
