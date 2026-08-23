#include "state.h"

uint8_t g_brightness = 200;
uint8_t g_fader[4] = {128, 128, 128, 128};
uint8_t g_mode = 0;

int g_weather_temp = -1;
int g_weather_humidity = -1;
int g_sensor_temp = -1;
int g_sensor_humidity = -1;
