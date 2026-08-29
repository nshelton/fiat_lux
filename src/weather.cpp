#include <WiFi.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "state.h"
#include "weather.h"
#include "secrets.h"

void weatherUpdate(uint32_t now, bool net_up) {
  if (!net_up) return;

  static uint32_t last = 0;
  if (last != 0 && now - last < WEATHER_MS) return;
  last = now;

  WiFiClient sock;
  HttpClient http(sock, "api.open-meteo.com", 80);
  http.setHttpResponseTimeout(5000);

  if (http.get(weather_api_path) != 0 || http.responseStatusCode() != 200) {
    Serial.println("weather request failed");
    http.stop();
    return;
  }
  http.skipResponseHeaders();

  JsonDocument filter;
  filter["current"]["temperature_2m"] = true;
  filter["current"]["relative_humidity_2m"] = true;
  filter["hourly"]["temperature_2m"] = true;
  filter["daily"]["sunrise"] = true;
  filter["daily"]["sunset"] = true;
  JsonDocument doc;
  if (deserializeJson(doc, http, DeserializationOption::Filter(filter)) == DeserializationError::Ok) {
    g_weather_temp = lroundf(doc["current"]["temperature_2m"].as<float>());
    g_weather_humidity = doc["current"]["relative_humidity_2m"].as<int>();
    JsonArray hourly = doc["hourly"]["temperature_2m"];
    if (hourly.size() == 24) {
      for (int i = 0; i < 24; i++) g_weather_hourly[i] = lroundf(hourly[i].as<float>());
      g_weather_hourly_ok = true;
    }
    // local iso8601, "2026-08-29T06:25" -- hour at 11, minute at 14
    const char* rise = doc["daily"]["sunrise"][0];
    const char* set = doc["daily"]["sunset"][0];
    if (rise && set && strlen(rise) >= 16 && strlen(set) >= 16) {
      g_sun_rise = atoi(rise + 11) * 60 + atoi(rise + 14);
      g_sun_set = atoi(set + 11) * 60 + atoi(set + 14);
    }
    Serial.print("weather: ");
    Serial.print(g_weather_temp);
    Serial.print("F ");
    Serial.print(g_weather_humidity);
    Serial.println("%");
  }
  http.stop();
}
