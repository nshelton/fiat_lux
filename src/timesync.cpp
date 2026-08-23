#include <WiFiNINA.h>
#include <TimeLib.h>
#include "config.h"
#include "timesync.h"

static WiFiUDP udp;
static bool begun = false;
static bool waiting = false;
static uint32_t sent_at = 0;
static uint32_t next_sync = 0;

static const int NTP_PACKET_SIZE = 48;
static uint8_t packet[NTP_PACKET_SIZE];

static void sendRequest() {
  memset(packet, 0, NTP_PACKET_SIZE);
  packet[0] = 0b11100011;  // LI, version, mode: client
  packet[2] = 6;
  packet[3] = 0xEC;
  packet[12] = 49;
  packet[13] = 0x4E;
  packet[14] = 49;
  packet[15] = 52;
  udp.beginPacket("pool.ntp.org", 123);
  udp.write(packet, NTP_PACKET_SIZE);
  udp.endPacket();
}

void timeSyncUpdate(uint32_t now, bool net_up) {
  if (!net_up) return;
  if (!begun) {
    udp.begin(2390);
    begun = true;
  }

  if (!waiting && (int32_t)(now - next_sync) >= 0) {
    sendRequest();
    waiting = true;
    sent_at = now;
  }

  if (!waiting) return;

  if (udp.parsePacket() >= NTP_PACKET_SIZE) {
    udp.read(packet, NTP_PACKET_SIZE);
    uint32_t secs = ((uint32_t)packet[40] << 24) | ((uint32_t)packet[41] << 16) |
                    ((uint32_t)packet[42] << 8) | packet[43];
    setTime(secs - 2208988800UL);  // clock runs in UTC, offset applied on display
    waiting = false;
    next_sync = now + NTP_RESYNC_MS;
    Serial.println("ntp synced");
  } else if (now - sent_at > 2000) {
    waiting = false;
    next_sync = now + 15000;
  }
}

static void put2(char* p, int v) {
  p[0] = '0' + v / 10 % 10;
  p[1] = '0' + v % 10;
}

static time_t nthSundayUTC(int y, int month, int nth, int hour_utc) {
  tmElements_t tm = {};
  tm.Year = CalendarYrToTm(y);
  tm.Month = month;
  tm.Day = 1;
  tm.Hour = hour_utc;
  time_t t = makeTime(tm);
  tm.Day = 1 + (8 - weekday(t)) % 7 + (nth - 1) * 7;
  return makeTime(tm);
}

// US DST rule, transitions at 2am local
static long utcOffsetSecs(time_t utc) {
  int y = year(utc);
  time_t dst_start = nthSundayUTC(y, 3, 2, 2 - TZ_STD_OFFSET);
  time_t dst_end = nthSundayUTC(y, 11, 1, 2 - TZ_DST_OFFSET);
  bool dst = utc >= dst_start && utc < dst_end;
  return (dst ? TZ_DST_OFFSET : TZ_STD_OFFSET) * 3600L;
}

void getDateTimeString(char* buf) {
  time_t t = now() + utcOffsetSecs(now());
  int hr = hour(t) % 12;
  if (hr == 0) hr = 12;
  put2(buf, hr);
  put2(buf + 2, minute(t));
  put2(buf + 4, second(t));
  memcpy(buf + 6, dayShortStr(weekday(t)), 3);
  put2(buf + 9, day(t));
  memcpy(buf + 11, monthShortStr(month(t)), 3);
  buf[14] = 0;
}
