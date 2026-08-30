#include <math.h>
#include "astro.h"

// truncated ephemerides: linear mean angles plus one periodic term each for
// the moon, two for the sun. A degree or two of error -- half a map pixel is
// 5.6 and a zodiac sign is 30, so both consumers have room to spare.

static const double DEG = M_PI / 180.0;

double astroDays(time_t t) { return t / 86400.0 - 10957.5; }

float sunEclipticLon(double d) {
  double g = fmod(357.529 + 0.98560028 * d, 360.0) * DEG;  // mean anomaly
  double lon = fmod(280.459 + 0.98564736 * d, 360.0) + 1.915 * sin(g) + 0.020 * sin(2 * g);
  return fmod(lon + 360.0, 360.0);
}

void moonEcliptic(double d, float* lon, float* lat) {
  double L = fmod(218.316 + 13.176396 * d, 360.0);         // mean longitude
  double M = fmod(134.963 + 13.064993 * d, 360.0) * DEG;   // mean anomaly
  double F = fmod(93.272 + 13.229350 * d, 360.0) * DEG;    // argument of latitude
  *lon = fmod(L + 6.289 * sin(M) + 360.0, 360.0);
  *lat = 5.128 * sin(F);
}

// Schlyter's mean orbital elements (epoch 1999 dec 31, secular drifts in N, w
// and e dropped -- they move well under a degree per decade). Kepler solve,
// heliocentric position, minus the earth's, projected to the ecliptic.
struct Elem {
  float N, i, w;   // node, inclination, arg of perihelion, degrees
  float a, e;      // AU, eccentricity
  float M0, Mr;    // mean anomaly at epoch and per day, degrees
};
static const Elem PLANETS[5] = {
    {48.3313f, 7.0047f, 29.1241f, 0.387098f, 0.205635f, 168.6562f, 4.0923344368f},   // mercury
    {76.6799f, 3.3946f, 54.8910f, 0.723330f, 0.006773f, 48.0052f, 1.6021302244f},    // venus
    {49.5574f, 1.8497f, 286.5016f, 1.523688f, 0.093405f, 18.6021f, 0.5240207766f},   // mars
    {100.4542f, 1.3030f, 273.8777f, 5.20256f, 0.048498f, 19.8950f, 0.0830853001f},   // jupiter
    {113.6634f, 2.4886f, 339.3939f, 9.55475f, 0.055546f, 316.9670f, 0.0334442282f},  // saturn
};

float planetEclipticLon(double d, int p) {
  const Elem& el = PLANETS[p];
  double M = fmod(el.M0 + el.Mr * (d + 1.5), 360.0) * DEG;  // +1.5: schlyter epoch
  double E = M + el.e * sin(M);
  for (int k = 0; k < 3; k++) E -= (E - el.e * sin(E) - M) / (1.0 - el.e * cos(E));
  double xv = el.a * (cos(E) - el.e), yv = el.a * sqrt(1.0 - el.e * el.e) * sin(E);
  double r = sqrt(xv * xv + yv * yv), u = atan2(yv, xv) + el.w * DEG;
  double N = el.N * DEG, ci = cos(el.i * DEG);
  double xh = r * (cos(N) * cos(u) - sin(N) * sin(u) * ci);
  double yh = r * (sin(N) * cos(u) + cos(N) * sin(u) * ci);

  double g = fmod(357.529 + 0.98560028 * d, 360.0) * DEG;  // earth, via the sun
  double re = 1.00014 - 0.01671 * cos(g) - 0.00014 * cos(2 * g);
  double le = (sunEclipticLon(d) + 180.0) * DEG;
  double lon = atan2(yh - re * sin(le), xh - re * cos(le)) / DEG;
  return fmod(lon + 360.0, 360.0);
}
