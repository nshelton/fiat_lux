#include <Arduino.h>
#include <time.h>
#include "config.h"
#include "timesync.h"

// The esp32 sntp client runs in the background and re-syncs on its own, so
// there is nothing to poll here. TZ_POSIX carries the DST rule, applied by
// localtime_r -- no hand-rolled transition dates.

void timeSyncUpdate(uint32_t, bool net_up) {
  static bool begun = false;
  static bool synced = false;
  if (!net_up) return;

  if (!begun) {
    configTzTime(TZ_POSIX, NTP_SERVER);
    begun = true;
  }

  if (!synced && time(nullptr) > 1700000000) {  // still 1970 until the first reply lands
    synced = true;
    Serial.println("ntp synced");
  }
}

void getDateTimeString(char* buf) {
  time_t t = time(nullptr);
  struct tm lt;
  localtime_r(&t, &lt);
  strftime(buf, 15, "%I%M%S%a%d%b", &lt);
}
