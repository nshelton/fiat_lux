#include <WiFiNINA.h>
#include <OSCMessage.h>
#include "config.h"
#include "state.h"
#include "osc.h"

static WiFiUDP udp;
static bool begun = false;

static void handle(OSCMessage& msg) {
  if (!msg.isFloat(0)) return;
  float val = msg.getFloat(0);
  char addr[64];
  msg.getAddress(addr);

  if (!strcmp(addr, "/1/fader1")) g_fader[0] = val * 255;
  else if (!strcmp(addr, "/1/fader2")) g_fader[1] = val * 255;
  else if (!strcmp(addr, "/1/fader3")) g_fader[2] = val * 255;
  else if (!strcmp(addr, "/1/fader4")) g_fader[3] = val * 255;
  else if (!strcmp(addr, "/1/fader5")) g_brightness = val * 255;
  else if (val == 1.0f) {
    if (!strcmp(addr, "/1/toggle1")) g_mode = 0;
    else if (!strcmp(addr, "/1/toggle2")) g_mode = 1;
    else if (!strcmp(addr, "/1/toggle3")) g_mode = 2;
    else if (!strcmp(addr, "/1/toggle4")) g_mode = 3;
  }
}

void oscUpdate(bool net_up) {
  if (!net_up) return;
  if (!begun) {
    udp.begin(OSC_PORT);
    begun = true;
  }

  int size = udp.parsePacket();
  if (size <= 0) return;
  static uint8_t buf[512];
  if (size > (int)sizeof(buf)) return;
  udp.read(buf, size);
  if (buf[0] != '/') return;

  OSCMessage msg;
  msg.fill(buf, size);
  if (!msg.hasError()) handle(msg);
}
