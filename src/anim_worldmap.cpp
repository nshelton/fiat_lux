#include <math.h>
#include <time.h>
#include "anim.h"
#include "astro.h"
#include "matrix.h"

// world map with live day/night shading. One cell is 11.25 degrees of the
// equirectangular earth: row 0 is the arctic, col 0 starts at 180W, so the
// terminator sweeps right-to-left at one column per 45 minutes. The sun's
// position comes straight from UTC -- declination from the day of year,
// subsolar longitude from the time -- no fetch, never stale.

static const char* const MAP[HEIGHT] = {
    ".........#####..................",  // 84N ellesmere, north greenland
    ".#############...###############",  // 73N arctic rim, bering strait gap
    ".#######.##.#.##################",  // 62N hudson bay notch, s greenland, iceland
    "....#######....##############.#.",  // 51N
    ".....#####.....##############...",  // 39N
    ".....####......############.....",  // 28N
    "......####....#######.#####.....",  // 17N sahel, arabia, india
    "........####...######.#.###.....",  //  6N
    ".........####....###.....#####..",  //  6S amazon, congo, indonesia
    ".........####....####......##...",  // 17S madagascar, north australia
    ".........###.....##.......####..",  // 28S
    ".........##.................##.#",  // 39S new zealand
    ".........#......................",  // 51S patagonia
    "..........#.....................",  // 62S antarctic peninsula
    "..#########..##################.",  // 73S ross and weddell notches
    "################################",  // 84S
};

static const CRGB OCEAN_DAY(0, 70, 180), OCEAN_NIGHT(0, 0, 24);
static const CRGB LAND_DAY(50, 210, 60), LAND_NIGHT(18, 28, 0);

struct WorldMapAnim : Animation {
  void frame(uint32_t) override {
    time_t t = time(nullptr);
    struct tm utc;
    gmtime_r(&t, &utc);
    bool synced = t > 1700000000;  // still 1970 = everything daylit, no sun dot

    static const float DEG = PI / 180.0f;
    float h = utc.tm_hour + utc.tm_min / 60.0f + utc.tm_sec / 3600.0f;
    float decl_deg = -23.44f * cosf(2 * PI * (utc.tm_yday + 10) / 365.0f);
    float sun_lon = (180.0f - 15.0f * h) * DEG;
    float sd = sinf(decl_deg * DEG), cd = cosf(decl_deg * DEG);

    float east[WIDTH];  // cos(lon - sun_lon) per column
    for (int c = 0; c < WIDTH; c++)
      east[c] = cosf((11.25f * (c + 0.5f) - 180.0f) * DEG - sun_lon);

    for (int r = 0; r < HEIGHT; r++) {
      float lat = (90.0f - 11.25f * (r + 0.5f)) * DEG;
      float sl = sinf(lat), cl = cosf(lat);
      for (int c = 0; c < WIDTH; c++) {
        // cosine of the solar zenith angle; +-0.1 around zero is twilight
        float z = synced ? sl * sd + cl * cd * east[c] : 1.0f;
        float d = (z + 0.1f) * 5.0f;
        d = d < 0 ? 0 : (d > 1 ? 1 : d);
        bool land = MAP[r][c] == '#';
        CRGB col = blend(land ? LAND_NIGHT : OCEAN_NIGHT,
                         land ? LAND_DAY : OCEAN_DAY, (uint8_t)(d * 255));
        setPixel(c, HEIGHT - 1 - r, col);
      }
    }

    if (synced) {
      int sc = (int)((180.0f - 15.0f * h + 180.0f) / 11.25f) & (WIDTH - 1);
      int sr = (int)((90.0f - decl_deg) / 11.25f);
      setPixel(sc, HEIGHT - 1 - sr, CRGB(255, 200, 0));

      // sublunar point: ecliptic position from astro.cpp, then equatorial
      // and minus sidereal time. Drawn after the sun so an eclipse does what
      // an eclipse does.
      double days = astroDays(t);
      float mlon_deg, mlat_deg;
      moonEcliptic(days, &mlon_deg, &mlat_deg);
      float lam = mlon_deg * DEG, bet = mlat_deg * DEG;
      const float OBL = 23.439f * DEG;  // obliquity of the ecliptic
      float mdec = asinf(sinf(bet) * cosf(OBL) + cosf(bet) * sinf(OBL) * sinf(lam));
      float mra = atan2f(sinf(lam) * cosf(OBL) - tanf(bet) * sinf(OBL), cosf(lam));
      float gmst = fmod(280.147 + 360.9856235 * days, 360.0) * DEG;
      float mlon = fmodf((mra - gmst) / DEG + 540.0f + 720.0f, 360.0f) - 180.0f;
      int mc = (int)((mlon + 180.0f) / 11.25f) & (WIDTH - 1);
      int mr = (int)((90.0f - mdec / DEG) / 11.25f);
      setPixel(mc, HEIGHT - 1 - mr, CRGB(110, 110, 130));
    }
  }
};

static WorldMapAnim s_anim;
Animation* const anim_worldmap = &s_anim;
