#include "secrets.h"
#include <OSCMessage.h>

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