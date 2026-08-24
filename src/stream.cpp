#include <WiFi.h>
#include "config.h"
#include "matrix.h"
#include "stream.h"

// packet: 'F' 'L' seq idx_hi idx_lo count flags, then count RGB triplets.
// idx is a raster pixel index (y * WIDTH + x); flags bit0 closes a frame.

static WiFiUDP udp;
static bool begun = false;
static uint32_t last_frame = 0;
static uint8_t buf[STREAM_BUF];
static bool http_pending = false;

// browsers cannot open a udp socket, so the control page posts whole frames to
// /frame instead; http.cpp fills the pixels and hands off the show to here
void streamHttpFrame(uint32_t now) {
  last_frame = now;
  http_pending = true;
}

bool streamActive(uint32_t now) {
  return last_frame != 0 && now - last_frame < STREAM_TIMEOUT_MS;
}

bool streamUpdate(uint32_t now, bool net_up) {
  if (!net_up) return false;
  if (!begun) {
    udp.begin(STREAM_PORT);
    begun = true;
  }

  bool complete = http_pending;
  http_pending = false;
  int avail;
  // drain everything queued this pass; showing only the last frame drops stale ones
  while ((avail = udp.parsePacket()) > 0) {
    int len = udp.read(buf, avail > (int)sizeof(buf) ? sizeof(buf) : avail);
    if (len < STREAM_HEADER || buf[0] != 'F' || buf[1] != 'L') continue;

    int count = buf[5];
    if (len < STREAM_HEADER + count * 3) continue;  // truncated, drop it

    int idx = (buf[3] << 8) | buf[4];
    const uint8_t* p = buf + STREAM_HEADER;
    for (int i = 0; i < count && idx + i < NUM_LEDS; i++, p += 3)
      setPixel((idx + i) % WIDTH, (idx + i) / WIDTH, CRGB(p[0], p[1], p[2]));

    if (buf[6] & 1) complete = true;
  }

  if (complete) last_frame = now;
  return complete;
}
