#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>

#include <Adafruit_NeoPixel.h>
#include <NTPClient.h>

enum State {
  SHOW_IP = 0,
  BLANK = 1,
  TIME = 2,
  OSC = 3
};

char ssid[] = "0(+___+)0";     // your network SSID (name)
char pass[] = "iamthewalrus";  // your network password

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;
// const IPAddress outIp(192,168,0,74);        // remote IP (not needed for receive)
// const unsigned int outPort = 9000;          // remote port (not needed for receive)
const unsigned int localPort = 8000;  // local port to listen for UDP packets (here's where we send the packets)

OSCErrorCode error;

#define BRIGHTNESS 10
#define NUM_LEDS 300

uint32_t leds[NUM_LEDS];

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "time.nist.gov", -8 * 60 * 60);  // -7 hours offset

int WIDTH = 30;
int HEIGHT = 10;

#define PI 3.1415

#define LED_DATA_PIN 12
#define BUTTON_PIN 13

String myIP;

State g_state = BLANK;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(30 * 10, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);


void setup() {

  Serial.begin(921600);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  myIP = WiFi.localIP().toString();

  Serial.println(myIP);

  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  #ifdef ESP32
  Serial.println(localPort);
  #else
  Serial.println(Udp.localPort());
  #endif

  timeClient.begin();

  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();


  pinMode(BUTTON_PIN, INPUT);

  g_state = SHOW_IP;
}


#define MAX_OSC_ADDR_LENGTH 64
#define MAX_OSC_MSG_LENGTH 16
#define OSC_FLOAT_LENGTH 4
char adresStr[MAX_OSC_ADDR_LENGTH];
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

void showMessage(OSCMessage &msg, int addrOffset) {
  msg.getAddress(adresStr);

  float val = 0;

  if (msg.isFloat(0)) {
    val = msg.getFloat(0);

    if (strcmp(adresStr, BRIGHTNESS_FADER) == 0) {
      g_brightness = (uint8_t)(val * 255);
    }
    if (strcmp(adresStr, MODE_1_TOGGLE) == 0) {
      g_state1 = val == 1.0;
    }
    if (strcmp(adresStr, MODE_2_TOGGLE) == 0) {
      g_state2 = val == 1.0;
    }
    if (strcmp(adresStr, MODE_3_TOGGLE) == 0) {
      g_state3 = val == 1.0;
    }
    if (strcmp(adresStr, MODE_4_TOGGLE) == 0) {
      g_state4 = val == 1.0;
    }
  }
}

void readOSC() {
  OSCMessage msgIN;
  int size;
  if ((size = Udp.parsePacket()) > 0) {
    byte udpData[size];
    for (int i = 0; i < size; i++) udpData[i] = Udp.read();
    // if data begins with / it is a message
    if (udpData[0] == 47) {
      msgIN.fill(udpData, size);
      if (!msgIN.hasError()) {
        msgIN.route("/*", showMessage);
      }
    }
  }
}


uint32_t packColor(char r, char g, char b) {
  return ((uint8_t)r << 16) + ((uint8_t)g << 8) + (uint8_t)b;
}

bool _0[] = { 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1 };
bool _1[] = { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
bool _2[] = { 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1 };
bool _3[] = { 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1 };
bool _4[] = { 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1 };
bool _5[] = { 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1 };
bool _6[] = { 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1 };
bool _7[] = { 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0 };
bool _8[] = { 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1 };
bool _9[] = { 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1 };
bool _dot[] = { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 };


int putChar(int n, int x, int y, int scale = 1, uint32_t col = 255) {

  //TODO : make width do something
  int width = 1;
  auto C = _dot;

  switch (n) {
    case 0: C = _0; break;
    case 1: C = _1; break;
    case 2: C = _2; break;
    case 3: C = _3; break;
    case 4: C = _4; break;
    case 5: C = _5; break;
    case 6: C = _6; break;
    case 7: C = _7; break;
    case 8: C = _8; break;
    case 9: C = _9; break;
  }

  for (int dx = 0; dx < 3; dx++) {
    for (int dy = 0; dy < 5 * scale; dy++) {

      // leds[i] = C[i-start] ? getColor(t) :  CRGB(0,0,0);
      float t = 0;
      if (C[dx * 5 + dy / scale]) {
        setPixel(x + dx, y + dy, col);
      }
    }
  }
  return width;
}
void setPixel(int x, int y, uint32_t col) {
  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
    return;
  }

  int xx = x;
  if (y % 2 == 1) {
    xx = WIDTH - 1 - x;
  }

  int idx = y * WIDTH + xx;
  leds[idx] = col;
}

void clear() {
  for (int i = 0; i < NUM_LEDS; i++) { leds[i] = 0; }
}

void show() {
  for (int i = 0; i < NUM_LEDS; i++) { strip.setPixelColor(i, leds[i]); }
  strip.show();
}

float palette_a[] = {0.5, 0.5, 0.5};
float palette_b[] = {0.5, 0.5, 0.5};

float palette_c[] = {1, 1, 1};
float palette_d[] = {0.3, 0.3, 0.3};
float g_time = 0;

uint32_t getColor(float t) {
  float cr = palette_a[0] + palette_b[0] * cos(2 * PI * (palette_c[0] * t + palette_d[0]));
  float cg = palette_a[1] + palette_b[1] * cos(2 * PI * (palette_c[1] * t + palette_d[1]));
  float cb = palette_a[2] + palette_b[2] * cos(2 * PI * (palette_c[2] * t + palette_d[2]));

  cr = constrain( cr * 255, 0, 255);
  cg = constrain( cg * 255, 0, 255);
  cb = constrain( cb * 255, 0, 255);

  return packColor(cr,cg,cb);
}
void displayGradient() {

  for (int x = 0; x < WIDTH; x ++) {
    float t = (float)x / WIDTH + g_time;

    for (int y = 0; y < HEIGHT; y ++) {
      float tx = t + 0.1 * (random(100) / 50.0) / WIDTH;
      uint32_t col =  getColor(tx);

      setPixel(x,y, col);
    }
  }

}
// char timeString[8];

void displayTime() {
  // int sec = timeClient.getSeconds();
  // int hr = timeClient.getHours();
  // int min = timeClient.getMinutes();
  // sprintf (timeString, "%d.%d.%d", hr, min, sec);
  timeClient.update();
  String timeString = timeClient.getFormattedTime();
  // Serial.println(timeString);

  int x = 0;
  int y = 0;

  for (size_t i = 0; i < 8; i++) {
    if (timeString[i] > '9') {
      x -= 1;
    }
    putChar(timeString[i] - '0', x, y, 2, packColor(255, 200, 100));
    if (timeString[i] > '9') {
      x += 3;
    } else {
      x += 4;
    }
  }
}

void displayIP() {

  int x = 0;
  int y = 0;
  for (size_t i = 0; i < myIP.length(); i++) {
    putChar(myIP[i] - '0', x, y);
    x += 4;
    if (i == 7) {
      x = 0;
      y = 5;
    }
  }
}

void loop() {
  clear();

  if (g_state1) {
    displayIP();
  }

  if (g_state2) {
    displayGradient();
  }

  if (g_state3) {
    displayTime();
  }

  show();
  delay(50);
  readOSC();

  strip.setBrightness(g_brightness);
}
