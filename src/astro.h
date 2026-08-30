#pragma once
#include <time.h>

double astroDays(time_t t);   // days since J2000
float sunEclipticLon(double d);                        // degrees, 0 = vernal equinox
void moonEcliptic(double d, float* lon, float* lat);   // degrees, lon in [0,360)
float planetEclipticLon(double d, int p);  // 0 mercury .. 4 saturn; geocentric, degrees
