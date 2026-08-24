#include <Preferences.h>
#include "config.h"
#include "state.h"
#include "prefs.h"

static Preferences prefs;

void prefsLoad() {
  prefs.begin("fiatlux", false);
  g_fg = prefs.getUInt("fg", g_fg);
  g_bg = prefs.getUInt("bg", g_bg);
}

// The picker sends a new colour on every mouse move, and NVS has a finite
// number of erase cycles, so nothing is written until the colour has held
// still for PREFS_SETTLE_MS.
void prefsUpdate(uint32_t now) {
  static uint32_t seen_fg = 0, seen_bg = 0, changed_at = 0;
  static bool primed = false;

  if (!primed) {
    seen_fg = g_fg;
    seen_bg = g_bg;
    primed = true;
    return;
  }

  if (g_fg != seen_fg || g_bg != seen_bg) {
    seen_fg = g_fg;
    seen_bg = g_bg;
    changed_at = now | 1;  // never 0, that is the idle marker
    return;
  }

  if (!changed_at || now - changed_at < PREFS_SETTLE_MS) return;
  changed_at = 0;
  prefs.putUInt("fg", g_fg);
  prefs.putUInt("bg", g_bg);
  Serial.println("colors saved");
}
