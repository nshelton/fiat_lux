#include <WiFiNINA.h>
#include "config.h"
#include "matrix.h"
#include "stream.h"

// packet: 'F' 'L' seq idx_hi idx_lo count flags, then count RGB triplets.
// idx is a raster pixel index (y * WIDTH + x); flags bit0 closes a frame.
//
// parsePacket() reports every byte the NINA has queued for the socket, not one
// datagram, so frames arrive back-to-back and can split across reads. Hence the
// count in the header: the parser walks whole packets and carries the remainder
// into the next read. Never bail with bytes still queued -- the next
// parsePacket() discards them a byte at a time while more keep arriving, which
// starves the render loop and the HTTP server until the panel looks hung.

static WiFiUDP udp;
static bool begun = false;
static uint32_t last_frame = 0;

static uint8_t buf[STREAM_BUF];
static int held = 0;  // bytes carried over from the previous read

bool streamActive(uint32_t now) {
  return last_frame != 0 && now - last_frame < STREAM_TIMEOUT_MS;
}

static bool consume(int len) {
  int o = 0;
  bool complete = false;

  while (len - o >= STREAM_HEADER) {
    const uint8_t* h = buf + o;
    if (h[0] != 'F' || h[1] != 'L') {
      o++;  // garbage or a lost header, walk to the next magic
      continue;
    }
    int count = h[5];
    int size = STREAM_HEADER + count * 3;
    if (len - o < size) break;  // tail of a split packet, wait for the rest

    int idx = (h[3] << 8) | h[4];
    const uint8_t* p = h + STREAM_HEADER;
    for (int i = 0; i < count && idx + i < NUM_LEDS; i++, p += 3)
      setPixel((idx + i) % WIDTH, (idx + i) / WIDTH, CRGB(p[0], p[1], p[2]));

    if (h[6] & 1) complete = true;
    o += size;
  }

  held = len - o;
  if (held) memmove(buf, buf + o, held);
  return complete;
}

bool streamUpdate(uint32_t now, bool net_up) {
  if (!net_up) return false;
  if (!begun) {
    udp.begin(STREAM_PORT);
    begun = true;
  }

  bool complete = false;
  int avail = udp.parsePacket();
  while (avail > 0 && held < (int)sizeof(buf)) {
    int n = udp.read(buf + held, sizeof(buf) - held);
    if (n <= 0) break;
    avail -= n;
    complete |= consume(held + n);
  }
  if (held >= (int)sizeof(buf)) held = 0;  // never resynced, drop and start over

  // many frames may land in one pass; showing only the last drops stale ones
  if (complete) last_frame = now;
  return complete;
}
