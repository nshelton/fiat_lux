#pragma once
#include <stdint.h>

extern uint8_t g_brightness;  // master brightness, HTTP bri=
extern uint8_t g_fader;       // HTTP f1= -- test pattern select, ticker speed
extern uint8_t g_mode;        // active animation, HTTP mode=

extern uint32_t g_fg;         // 0xRRGGBB, HTTP fg= -- text and live cells
extern uint32_t g_bg;         // 0xRRGGBB, HTTP bg= -- everything behind them

extern int g_weather_temp;    // deg F from weatherapi.com, -1 = unknown
extern int g_weather_humidity;
extern int g_sensor_temp;     // deg F from the AHT sensor
extern int g_sensor_humidity;
