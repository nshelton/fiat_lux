#include "secrets.h"
#include <OSCMessage.h>


uint8_t g_fader_1 = 128;
uint8_t g_fader_2 = 128;
uint8_t g_fader_3 = 128;
uint8_t g_fader_4 = 128;


void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(19200);
  // while (!Serial);
  // blocks until connection
  connectWifi(sec_ssid, sec_pass);
  getWifiStatus();
  setupLED();
  // delay(4000);

  setupOSCRoute();
  setupTime();
}

void loop() {
  readOSC();
  updateLED();
  // updateTime();
  // getWifiStatus();
  delay(33);

}