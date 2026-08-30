#pragma once
#include <stdint.h>

extern uint8_t g_brightness;  // master brightness, HTTP bri=
extern uint8_t g_fader;       // panel env test pattern select; no HTTP param reads it
extern uint8_t g_mode;        // active animation, HTTP mode=

extern uint32_t g_fg;         // 0xRRGGBB, HTTP fg= -- text and live cells
extern uint32_t g_bg;         // 0xRRGGBB, HTTP bg= -- everything behind them

extern int g_weather_temp;    // deg F from open-meteo, -1 = unknown
extern int g_weather_humidity;
extern int g_weather_hourly[24];  // today's forecast, deg F per hour, local midnight to midnight
extern int g_humidity_hourly[24]; // same, % relative humidity
extern bool g_weather_hourly_ok;  // covers both hourly arrays, false until the first successful fetch
extern int g_sun_rise;            // minutes since local midnight, -1 = unknown
extern int g_sun_set;
extern int g_aqi;                 // current US AQI from open-meteo, -1 = unknown
extern int g_aqi_hourly[24];      // today's hourly US AQI, local midnight to midnight
extern bool g_aqi_hourly_ok;      // false until the first successful fetch
extern int g_sensor_temp;     // deg F from the AHT sensor
extern int g_sensor_humidity;
