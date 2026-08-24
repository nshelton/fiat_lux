#include <FastLED.h>
#include "config.h"
#include "state.h"
#include "matrix.h"
#include "anim.h"

// Panel + level-shifter validation. No wifi is linked into this env, so the
// only things that can glitch the display are the 5V data path and FastLED's
// RMT timing. Clean at brightness 1 means the voltage problem is closed.
//
// Runs the index chase (led 0 red, white walker in wiring order) up a
// power-of-two brightness ladder, announcing each step on serial so a glitch
// can be pinned to a number.

static const uint8_t LADDER[] = {1, 2, 4, 8, 16, 32, 64, 128, 255};
static const uint32_t STEP_MS = 4000;

void setup() {
  Serial.begin(115200);
  matrixSetup();
  g_fader[0] = 0;  // index chase
  Serial.println("panel test");
}

void loop() {
  static uint32_t last_step = 0;
  static int step = -1;
  uint32_t now = millis();

  if (step < 0 || now - last_step >= STEP_MS) {
    last_step = now;
    step = (step + 1) % (int)sizeof(LADDER);
    Serial.print("brightness ");
    Serial.println(LADDER[step]);
  }

  FastLED.setBrightness(LADDER[step]);
  anim_test->frame(now);
  FastLED.show();
  delay(FRAME_MS);
}
