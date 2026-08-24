#include <Adafruit_AHTX0.h>
#include "config.h"
#include "state.h"
#include "sensor.h"

static Adafruit_AHTX0 aht;
static bool present = false;

// a live sensor board holds both lines high through its pullups; without one
// there is nothing on the bus to talk to
static bool i2cAlive() {
  pinMode(SDA, INPUT);
  pinMode(SCL, INPUT);
  return digitalRead(SDA) && digitalRead(SCL);
}

void sensorSetup() {
  if (!i2cAlive()) {
    Serial.println("no I2C pullups, sensor skipped");
    return;
  }
  present = aht.begin();
  if (!present) Serial.println("AHT sensor not found");
}

void sensorUpdate(uint32_t now) {
  static uint32_t last = 0;
  if (last != 0 && now - last < SENSOR_MS) return;
  last = now;

  if (!present) {
    if (!i2cAlive()) return;
    present = aht.begin();
    if (!present) return;
  }

  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  g_sensor_temp = temp.temperature * 9 / 5 + 32;
  g_sensor_humidity = humidity.relative_humidity;
}
