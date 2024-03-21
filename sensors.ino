#include <Wire.h>
#include "Adafruit_AHTX0.h"

Adafruit_AHTX0 aht;


unsigned long last_sensor_update = 0;
int sensor_update_freq = 1000 * 60;  ///1m


void init_sensors() {
  if (!aht.begin()) {
    Serial.println("Could not find AHT sensor, check wiring!");
    while (1)
      ;
  }
  Serial.println("AHT sensor found!");
}

void update_sensors() {

  if (millis() - last_sensor_update > sensor_update_freq) {
    // Read temperature and humidity data
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);
    g_sensor_temperature = (temp.temperature * 9 / 5) + 32;
    g_sensor_humidity = humidity.relative_humidity;

    Serial.print("Temperature: ");
    Serial.print(g_sensor_temperature);
    Serial.print(" °f, Humidity: ");
    Serial.print(g_sensor_humidity);
    Serial.println("%");
    last_sensor_update = millis();
  }
}