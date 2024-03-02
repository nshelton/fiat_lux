#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;
// const IPAddress outIp(192,168,0,74);        // remote IP (not needed for receive)
// const unsigned int outPort = 9000;          // remote port (not needed for receive)
const unsigned int osc_port = 8000;  // local port to listen for UDP packets (here's where we send the packets)

OSCErrorCode error;

#define MAX_OSC_ADDR_LENGTH 64
#define MAX_OSC_MSG_LENGTH 16
#define OSC_FLOAT_LENGTH 4

char address_buf[MAX_OSC_ADDR_LENGTH];
char oscmsg[MAX_OSC_MSG_LENGTH];
char oscval[10];

char debugString[100];
char message[100];

#define BRIGHTNESS_FADER "/1/fader5"
uint8_t g_brightness = 10;

#define MODE_1_TOGGLE "/1/toggle1"
#define MODE_2_TOGGLE "/1/toggle2"
#define MODE_3_TOGGLE "/1/toggle3"
#define MODE_4_TOGGLE "/1/toggle4"

bool g_state1 = false;
bool g_state2 = false;
bool g_state3 = false;
bool g_state4 = false;

// enum State {
//   SHOW_IP = 0,
//   BLANK = 1,
//   TIME = 2,
//   OSC = 3
// };
// State g_state = BLANK;

void onOSCMessage(OSCMessage &msg, int addrOffset) {
  msg.getAddress(address_buf);

  if (msg.isFloat(0)) {
    float val = msg.getFloat(0);
    Serial.print(address_buf);
    Serial.print(" ");
    Serial.println(val);

    if (strcmp(address_buf, BRIGHTNESS_FADER) == 0) {
      g_brightness = (uint8_t)(val * 255);
      setLEDBrightness(g_brightness);
    }

    if (strcmp(address_buf, "/1/fader1") == 0) {
      g_fader_1 = (uint8_t)(val * 255);
    }

    if (strcmp(address_buf, "/1/fader2") == 0) {
      g_fader_2 = (uint8_t)(val * 255);
    }

    if (strcmp(address_buf, "/1/fader3") == 0) {
      g_fader_3 = (uint8_t)(val * 255);
    }

    if (strcmp(address_buf, "/1/fader4") == 0) {
      g_fader_4 = (uint8_t)(val * 255);
    }

    if (strcmp(address_buf, MODE_1_TOGGLE) == 0) {
      g_state1 = val == 1.0;
    }
    if (strcmp(address_buf, MODE_2_TOGGLE) == 0) {
      g_state2 = val == 1.0;
    }
    if (strcmp(address_buf, MODE_3_TOGGLE) == 0) {
      g_state3 = val == 1.0;
    }
    if (strcmp(address_buf, MODE_4_TOGGLE) == 0) {
      g_state4 = val == 1.0;
    }
  }
}

void readOSC() {
  OSCMessage msgIN;
  int size;
  if ((size = Udp.parsePacket()) > 0) {
    byte udpData[size];
    Serial.println(size);
    for (int i = 0; i < size; i++) udpData[i] = Udp.read();
    // if data begins with / it is a message
    if (udpData[0] == 47) {
      
      msgIN.fill(udpData, size);
      if (!msgIN.hasError()) {
        msgIN.route("/*", onOSCMessage);
      }
    }
  }
}

void setupOSCRoute() {

  Serial.println("Starting OSC UDP");
  Udp.begin(osc_port);
  Serial.print("local port: ");
  Serial.println(osc_port);
}