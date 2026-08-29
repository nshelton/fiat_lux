#include <WiFi.h>
#include "config.h"
#include "state.h"
#include "matrix.h"
#include "stream.h"
#include "http.h"

static WiFiServer server(HTTP_PORT);
static bool begun = false;

static const char PAGE[] = R"HTML(<!doctype html><meta charset=utf-8>
<meta name=viewport content="width=device-width,initial-scale=1"><title>fiat lux</title>
<style>
body{background:#0b0b0c;color:#e8e6e3;font:15px/1.4 ui-monospace,Menlo,monospace;margin:0;padding:24px}
main{max-width:380px;margin:0 auto}
h1{font-size:13px;letter-spacing:.25em;text-transform:uppercase;color:#7a7a7a;margin:0 0 20px}
h2{font-size:11px;letter-spacing:.25em;text-transform:uppercase;color:#5a5a5a;margin:0 0 10px;font-weight:400}
#modes{display:grid;grid-template-columns:repeat(2,1fr);gap:8px;margin-bottom:26px}
button{background:#161618;color:#bdbdbd;border:1px solid #2a2a2e;border-radius:6px;padding:11px;font:inherit;cursor:pointer}
button.on{background:#e8e6e3;color:#0b0b0c;border-color:#e8e6e3}
#env{display:grid;grid-template-columns:auto 1fr 1fr;gap:3px 14px;font-size:13px;color:#7a7a7a;margin-bottom:22px}
#env b{color:#e8e6e3;font-weight:400}
#cols{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-bottom:26px}
#cols label{margin-bottom:0}
input[type=color]{height:38px;padding:2px;border:1px solid #2a2a2e;border-radius:6px;background:#161618;cursor:pointer}
label{display:block;margin-bottom:18px}
span{display:flex;justify-content:space-between;font-size:12px;color:#7a7a7a;margin-bottom:6px}
i{font-style:normal;color:#e8e6e3}
input{width:100%;accent-color:#e8e6e3}
</style>
<main><h1>fiat lux</h1><div id=env></div><div id=modes></div>
<h2>color</h2><div id=cols>
<label><span>foreground</span><input type=color id=cfg></label>
<label><span>background</span><input type=color id=cbg></label></div>
<div id=faders></div></main>
<script>
const M=['clock','wolfram','plasma','test','ticker','weather','humidity'],
F=['brightness','fader'];
let s={mode:0,v:[200,128],fg:'ffffff',bg:'000000',t:[-1,-1,-1,-1],heap:0},dirty=false,busy=false;
cfg.oninput=()=>{s.fg=cfg.value.slice(1);dirty=true};
cbg.oninput=()=>{s.bg=cbg.value.slice(1);dirty=true};
M.forEach((n,i)=>{let b=document.createElement('button');b.textContent=n;
b.onclick=()=>{s.mode=i;draw();dirty=true};modes.append(b)});
F.forEach((n,i)=>{let l=document.createElement('label');
l.innerHTML='<span>'+n+'<i id=v'+i+'></i></span>';
let r=document.createElement('input');r.type='range';r.min=0;r.max=255;r.id='r'+i;
r.oninput=()=>{s.v[i]=+r.value;draw();dirty=true};l.append(r);faders.append(l)});
function draw(){[...modes.children].forEach((b,i)=>b.className=i==s.mode?'on':'');
s.v.forEach((x,i)=>{r=window['r'+i];r.value=x;window['v'+i].textContent=x});
cfg.value='#'+s.fg;cbg.value='#'+s.bg}
setInterval(async()=>{if(!dirty||busy)return;dirty=false;busy=true;
try{await fetch('/set?mode='+s.mode+'&bri='+s.v[0]+'&f1='+s.v[1]+'&fg='+s.fg+'&bg='+s.bg)}catch(e){}
busy=false},120);
function drawState(j){let f=v=>v<0?'--':v,t=j.t;
env.innerHTML='<span>out</span><b>'+f(t[0])+'&deg;F</b><b>'+f(t[1])+'%</b>'+
'<span>in</span><b>'+f(t[2])+'&deg;F</b><b>'+f(t[3])+'%</b>'+
'<span>mode</span><b>'+M[j.mode]+'</b><b></b>'+
'<span>bri/fader</span><b>'+j.v[0]+'</b><b>'+j.v[1]+'</b>'+
'<span>fg/bg</span><b>#'+j.fg+'</b><b>#'+j.bg+'</b>'+
'<span>heap</span><b>'+(j.heap?(j.heap/1024).toFixed(1)+'k':'--')+'</b><b></b>'}
drawState(s);
fetch('/state').then(r=>r.json()).then(j=>{s=j;draw();drawState(j)});draw();
// display only -- assigning into s here would fight a drag in progress
setInterval(()=>fetch('/state').then(r=>r.json()).then(j=>drawState(j)).catch(e=>{}),10000);
</script>)HTML";

static void sendBody(WiFiClient& c, const char* type, const char* body, size_t len) {
  char head[128];
  int hn = snprintf(head, sizeof(head),
                    "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %u\r\n"
                    "Connection: close\r\n\r\n",
                    type, (unsigned)len);
  c.write((const uint8_t*)head, hn);
  c.write((const uint8_t*)body, len);
}

static void sendState(WiFiClient& c) {
  char body[160];
  int n = snprintf(body, sizeof(body),
                   "{\"mode\":%u,\"v\":[%u,%u],\"fg\":\"%06lx\",\"bg\":\"%06lx\","
                   "\"t\":[%d,%d,%d,%d],\"heap\":%u}",
                   g_mode, g_brightness, g_fader,
                   (unsigned long)g_fg, (unsigned long)g_bg,
                   g_weather_temp, g_weather_humidity, g_sensor_temp, g_sensor_humidity,
                   (unsigned)ESP.getFreeHeap());
  sendBody(c, "application/json", body, n);
}

// Body is always a whole frame, so there is nothing to negotiate and no
// Content-Length to parse -- read exactly that many bytes or drop it. One
// deadline for the whole body, not per chunk: a client dripping a byte at a
// time would otherwise hold the loop open and starve the render. A dropped
// frame costs nothing, the next one is a frame time behind it.
static void recvFrame(WiFiClient& c) {
  static uint8_t body[NUM_LEDS * 3];
  size_t n = 0;
  uint32_t t0 = millis();
  while (n < sizeof(body) && millis() - t0 < HTTP_TIMEOUT_MS) {
    int r = c.read(body + n, sizeof(body) - n);
    if (r > 0) n += r;
    else if (!c.connected()) break;
  }
  if (n < sizeof(body)) return;  // short frame, drop it rather than show a torn one

  const uint8_t* p = body;
  for (int i = 0; i < NUM_LEDS; i++, p += 3)
    setRaster(i, CRGB(p[0], p[1], p[2]));
  streamHttpFrame(millis());
}

// -1 when the key is absent
static int param(const char* req, const char* key) {
  const char* p = strstr(req, key);
  if (!p) return -1;
  int v = atoi(p + strlen(key));
  return v < 0 ? 0 : (v > 255 ? 255 : v);
}

// -1 unless the key is followed by exactly six hex digits
static long paramHex(const char* req, const char* key) {
  const char* p = strstr(req, key);
  if (!p) return -1;
  p += strlen(key);
  long v = 0;
  for (int i = 0; i < 6; i++) {
    char c = p[i];
    int d = (c >= '0' && c <= '9')   ? c - '0'
            : (c >= 'a' && c <= 'f') ? c - 'a' + 10
            : (c >= 'A' && c <= 'F') ? c - 'A' + 10
                                     : -1;
    if (d < 0) return -1;
    v = v * 16 + d;
  }
  return v;
}

void httpUpdate(bool net_up) {
  if (!net_up) return;
  if (!begun) {
    server.begin();
    begun = true;
  }

  WiFiClient client = server.available();
  if (!client) return;

  // keep the request line, drain the headers
  char req[160];
  size_t n = 0;
  bool line_done = false;
  uint32_t last4 = 0, t0 = millis();
  while (client.connected() && millis() - t0 < HTTP_TIMEOUT_MS) {
    if (!client.available()) continue;
    char c = client.read();
    last4 = (last4 << 8) | (uint8_t)c;
    if (last4 == 0x0D0A0D0AUL) break;
    if (c == '\r' || c == '\n') line_done = true;
    else if (!line_done && n < sizeof(req) - 1) req[n++] = c;
  }
  req[n] = 0;

  // a request line cut short by the timeout would parse to truncated values
  // ("bri=90" read as "bri=9"), so only act once the whole line is in
  if (!line_done) {
    client.stop();
    return;
  }

  if (!strncmp(req, "GET /set", 8)) {
    int v;
    if ((v = param(req, "mode=")) >= 0) g_mode = v;
    if ((v = param(req, "bri=")) >= 0) g_brightness = v;
    if ((v = param(req, "f1=")) >= 0) g_fader = v;
    long c;
    if ((c = paramHex(req, "fg=")) >= 0) g_fg = c;
    if ((c = paramHex(req, "bg=")) >= 0) g_bg = c;
    sendState(client);
  } else if (!strncmp(req, "GET /state", 10)) {
    sendState(client);
  } else if (!strncmp(req, "POST /frame", 11)) {
    recvFrame(client);
    client.print("HTTP/1.1 204 No Content\r\nConnection: close\r\n\r\n");
  } else if (!strncmp(req, "GET / ", 6)) {
    sendBody(client, "text/html", PAGE, sizeof(PAGE) - 1);
  } else {
    client.print("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");
  }

  client.stop();
}
