#include <Adafruit_AHTX0.h>
#include "config.h"
#include "state.h"
#include "sensor.h"

static Adafruit_AHTX0 aht;
static bool present = false;

void sensorSetup() {
  present = aht.begin();
  if (!present) Serial.println("AHT sensor not found");
}

void sensorUpdate(uint32_t now) {
  static uint32_t last = 0;
  if (last != 0 && now - last < SENSOR_MS) return;
  last = now;

  if (!present) {
    present = aht.begin();
    if (!present) return;
  }

  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  g_sensor_temp = temp.temperature * 9 / 5 + 32;
  g_sensor_humidity = humidity.relative_humidity;
}
