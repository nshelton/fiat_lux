#include <WiFi.h>
#include <ArduinoOTA.h>
#include "config.h"
#include "net.h"
#include "secrets.h"

void netSetup() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  WiFi.setSleep(false);  // modem sleep adds latency to the http server and to streaming
}

bool netUpdate(uint32_t now) {
  static uint32_t last_attempt = 0;
  static bool ota_ready = false;

  if (WiFi.status() != WL_CONNECTED) {
    if (last_attempt == 0 || now - last_attempt > 10000) {
      Serial.print("connecting to ");
      Serial.println(sec_ssid);
      WiFi.begin(sec_ssid, sec_pass);  // non-blocking on esp32; netUpdate polls status
      last_attempt = now;
    }
    return false;
  }

  if (!ota_ready) {
    ArduinoOTA.setHostname(HOSTNAME);
    ArduinoOTA.setPassword(ota_password);
    ArduinoOTA.begin();  // also advertises HOSTNAME.local over mdns
    ota_ready = true;
    Serial.print("wifi up, IP: ");
    Serial.println(WiFi.localIP());
  }
  ArduinoOTA.handle();
  return true;
}
