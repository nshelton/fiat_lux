#include <HttpClient.h>
#include <WiFiNINA.h>
#include <ArduinoJson.h>

// Number of milliseconds to wait without receiving any data before we give up
const int kNetworkTimeout = 30*1000;
// Number of milliseconds to wait if no data is available before trying again
const int kNetworkDelay = 1000;
DynamicJsonDocument weather_json(2048);

uint32_t last_weather_time = 0;

const int k_weatherUpdateTime = 1000 * 30; // 30s 

void parseWeather() {

  // temp_f condition wind_mph pressure_mb pressure_in precip_in feelslike_c feelslike_f air_quality 

  // for (JsonPair kv : weather_json["current"].as<JsonObject>()) {
  //   Serial.print(kv.key().c_str());  
  //   Serial.print(" ");  
  // }

  // Serial.print("\n");  
  // for (JsonPair kv : weather_json["location"].as<JsonObject>()) {
  //   Serial.print(kv.key().c_str());  
  //   Serial.print(" ");  
  // }
  
  g_local_temperature = weather_json["current"]["feelslike_f"].as<int>();
  g_local_temperature = weather_json["current"]["temp_f"].as<int>();
  g_local_humidity = weather_json["current"]["humidity"].as<int>();

  last_weather_time = millis();
  Serial.println("weatherRequest ok:");
  Serial.print("g_local_temperature: ");
  Serial.print(g_local_temperature);
  Serial.print("g_local_humidity: ");
  Serial.print(g_local_humidity);
}

void updateWeather() {
  
  if ((millis() - last_weather_time) < k_weatherUpdateTime) {
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("wifi not connect. no weather yet");
    return;
  }


  WiFiClient c;
  HttpClient http(c);

  int err =0;
  err = http.get("api.weatherapi.com", "/v1/current.json?key=b0283b47668143a7bb630558241103&q=los%20angeles&aqi=yes");
  if (err == 0)
  { 
    err = http.responseStatusCode();
    if (err >= 0)
    {
      err = http.skipResponseHeaders();
      if (err >= 0)
      {
        deserializeJson(weather_json, http);
        parseWeather();
      }
      else
      {
        Serial.print("Failed to skip response headers: ");
        Serial.println(err);
      }
    }
    else
    {    
      Serial.print("Getting response failed: ");
      Serial.println(err);
    }
  }
  else
  {
    Serial.print("Connect failed: ");
    Serial.println(err);
  }

  http.stop();

}