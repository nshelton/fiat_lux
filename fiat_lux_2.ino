#include "secrets.h"
#include <OSCMessage.h>

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(19200);
  // while (!Serial);
  // blocks until connection
  connectWifi(sec_ssid, sec_pass);
  setupLED();
  setupOSCRoute();
  // pinMode(LED_BUILTIN, OUTPUT);
  getWifiStatus();
  // setupTime();
}

void loop() {

  updateLED();
  readOSC();
  // updateTime();
  // getWifiStatus();
  delay(10);

}