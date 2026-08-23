#pragma once
#include <stdint.h>

extern uint8_t g_brightness;  // master brightness, OSC /1/fader5
extern uint8_t g_fader[4];    // OSC /1/fader1-4, free params for animations
extern uint8_t g_mode;        // active animation, OSC /1/toggle1-4

extern int g_weather_temp;    // deg F from weatherapi.com, -1 = unknown
extern int g_weather_humidity;
extern int g_sensor_temp;     // deg F from the AHT sensor
extern int g_sensor_humidity;
