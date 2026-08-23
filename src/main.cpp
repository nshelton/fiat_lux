#include <FastLED.h>
#include "config.h"
#include "state.h"
#include "matrix.h"
#include "anim.h"
#include "net.h"
#include "osc.h"
#include "timesync.h"
#include "weather.h"
#include "sensor.h"

static Animation* const anims[] = {anim_clock, anim_ca, anim_plasma, anim_test};

void setup() {
  Serial.begin(115200);
  matrixSetup();
  netSetup();
  sensorSetup();
}

void loop() {
  uint32_t now = millis();

  bool net_up = netUpdate(now);
  oscUpdate(net_up);
  timeSyncUpdate(now, net_up);
  weatherUpdate(now, net_up);
  sensorUpdate(now);

  static uint32_t last_frame = 0;
  if (now - last_frame < FRAME_MS) return;
  last_frame = now;

  static uint8_t active = 255;
  uint8_t mode = g_mode < 4 ? g_mode : 0;
  if (mode != active) {
    active = mode;
    anims[active]->begin();
  }

  FastLED.setBrightness(g_brightness);
  anims[active]->frame(now);
  FastLED.show();
}
