#include <FastLED.h>
#include "config.h"
#include "state.h"
#include "matrix.h"
#include "anim.h"
#include "net.h"
#include "http.h"
#include "stream.h"
#include "timesync.h"
#include "transition.h"
#include "weather.h"
#include "sensor.h"
#include "prefs.h"

static Animation* const anims[] = {anim_clock,  anim_ca,      anim_plasma, anim_test,
                                   anim_scroll, anim_weather, anim_humidity};
static const uint8_t NUM_ANIMS = sizeof(anims) / sizeof(anims[0]);

void setup() {
  Serial.begin(115200);
  Serial.println("boot");
  prefsLoad();
  matrixSetup();
  Serial.println("matrix ok");
  netSetup();
  Serial.println("net ok");
  sensorSetup();
  Serial.println("sensor ok");
}

void loop() {
  uint32_t now = millis();

  bool net_up = netUpdate(now);
  httpUpdate(net_up);
  timeSyncUpdate(now, net_up);
  weatherUpdate(now, net_up);
  sensorUpdate(now);
  prefsUpdate(now);

  static uint8_t active = 255;
  bool streamed = streamUpdate(now, net_up);
  if (streamActive(now)) {
    active = 255;  // re-run begin() when the stream drops and animations resume
    transitionCancel();
    if (streamed) {
      FastLED.setBrightness(g_brightness);
      FastLED.show();
    }
    return;
  }

  static uint32_t last_frame = 0;
  if (now - last_frame < FRAME_MS) return;
  last_frame = now;

  uint8_t mode = g_mode < NUM_ANIMS ? g_mode : 0;
  if (mode != active) {
    // glitch-wipe the old frame out before the new mode draws in
    if (active != 255) {
      if (!transitionActive()) transitionStart();
      if (transitionFrame()) {
        FastLED.setBrightness(g_brightness);
        FastLED.show();
        return;
      }
    }
    active = mode;
    anims[active]->begin();
  }

  FastLED.setBrightness(g_brightness);
  anims[active]->frame(now);
  transitionPost();
  FastLED.show();
}
