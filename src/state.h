#pragma once
#include <stdint.h>

extern uint8_t g_brightness;  // master brightness, HTTP bri=
extern uint8_t g_fader[4];    // HTTP f1-f4, free params for animations
extern uint8_t g_mode;        // active animation, HTTP mode=

extern int g_weather_temp;    // deg F from weatherapi.com, -1 = unknown
extern int g_weather_humidity;
extern int g_sensor_temp;     // deg F from the AHT sensor
extern int g_sensor_humidity;
