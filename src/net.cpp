#include <WiFiNINA.h>
#include <ArduinoOTA.h>
#include "config.h"
#include "net.h"
#include "secrets.h"

void netSetup() {
  WiFi.setHostname(HOSTNAME);
  WiFi.setTimeout(0);  // begin() otherwise blocks up to 50s per attempt; netUpdate polls status
}

bool netUpdate(uint32_t now) {
  static uint32_t last_attempt = 0;
  static bool ota_ready = false;

  if (WiFi.status() != WL_CONNECTED) {
    if (last_attempt == 0 || now - last_attempt > 10000) {
      Serial.print("connecting to ");
      Serial.println(sec_ssid);
      WiFi.begin(sec_ssid, sec_pass);
      last_attempt = now;
    }
    return false;
  }

  if (!ota_ready) {
    ArduinoOTA.begin(WiFi.localIP(), "arduino", ota_password, InternalStorage);
    ota_ready = true;
    Serial.print("wifi up, IP: ");
    Serial.println(WiFi.localIP());
  }
  ArduinoOTA.poll();
  return true;
}
