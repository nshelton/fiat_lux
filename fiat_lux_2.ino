#include <FastLED.h>

#include "secrets.h"
#include <OSCMessage.h>

uint8_t g_fader_1 = 128;
uint8_t g_fader_2 = 128;
uint8_t g_fader_3 = 128;
uint8_t g_fader_4 = 128;

int g_local_temperature = -1;
int g_local_humidity = -1;

int g_sensor_temperature = -1;
int g_sensor_humidity = -1;


bool network_mutex = false;

void setup() {
  Serial.begin(19200);
  setupLED();

  //Initialize serial and wait for port to open:
  // while (!Serial); 
  // blocks until connection
  connectWifi(sec_ssid, sec_pass);
  getWifiStatus();
  // delay(4000);

  setupOSCRoute();
  setupTime();
  init_sensors();
}

void loop() {
  readOSC();
  updateLED();
  // updateTime();
  // getWifiStatus();
  updateWeather();
  delay(10);

  update_sensors();


}