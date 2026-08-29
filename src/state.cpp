#include "state.h"

uint8_t g_brightness = 200;
uint8_t g_fader = 128;
uint8_t g_mode = 0;

uint32_t g_fg = 0xFFFFFF;
uint32_t g_bg = 0x000000;

int g_weather_temp = -1;
int g_weather_humidity = -1;
int g_weather_hourly[24];
bool g_weather_hourly_ok = false;
int g_sun_rise = -1;
int g_sun_set = -1;
int g_sensor_temp = -1;
int g_sensor_humidity = -1;
