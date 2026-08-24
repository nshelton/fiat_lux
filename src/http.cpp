#include <WiFiNINA.h>
#include "config.h"
#include "state.h"
#include "http.h"

static WiFiServer server(HTTP_PORT);
static bool begun = false;

#define STR(x) #x
#define XSTR(x) STR(x)

static const char PAGE[] = R"HTML(<!doctype html><meta charset=utf-8>
<meta name=viewport content="width=device-width,initial-scale=1"><title>fiat lux</title>
<style>
body{background:#0b0b0c;color:#e8e6e3;font:15px/1.4 ui-monospace,Menlo,monospace;margin:0;padding:24px}
main{max-width:380px;margin:0 auto}
h1{font-size:13px;letter-spacing:.25em;text-transform:uppercase;color:#7a7a7a;margin:0 0 20px}
#modes{display:grid;grid-template-columns:repeat(2,1fr);gap:8px;margin-bottom:26px}
button{background:#161618;color:#bdbdbd;border:1px solid #2a2a2e;border-radius:6px;padding:11px;font:inherit;cursor:pointer}
button.on{background:#e8e6e3;color:#0b0b0c;border-color:#e8e6e3}
label{display:block;margin-bottom:18px}
span{display:flex;justify-content:space-between;font-size:12px;color:#7a7a7a;margin-bottom:6px}
i{font-style:normal;color:#e8e6e3}
input{width:100%;accent-color:#e8e6e3}
</style>
<main><h1>fiat lux</h1><div id=modes></div><div id=faders></div></main>
<script>
const M=['clock','wolfram','plasma','test','ticker'],
F=['brightness','fader 1','fader 2','fader 3','fader 4'];
let s={mode:0,v:[200,128,128,128,128]},dirty=false,busy=false;
M.forEach((n,i)=>{let b=document.createElement('button');b.textContent=n;
b.onclick=()=>{s.mode=i;draw();dirty=true};modes.append(b)});
F.forEach((n,i)=>{let l=document.createElement('label');
l.innerHTML='<span>'+n+'<i id=v'+i+'></i></span>';
let r=document.createElement('input');r.type='range';r.min=i?0:)HTML" XSTR(MIN_BRIGHTNESS) R"HTML(;r.max=255;r.id='r'+i;
r.oninput=()=>{s.v[i]=+r.value;draw();dirty=true};l.append(r);faders.append(l)});
function draw(){[...modes.children].forEach((b,i)=>b.className=i==s.mode?'on':'');
s.v.forEach((x,i)=>{r=window['r'+i];r.value=x;window['v'+i].textContent=x})}
setInterval(async()=>{if(!dirty||busy)return;dirty=false;busy=true;
try{await fetch('/set?mode='+s.mode+'&bri='+s.v[0]+'&f1='+s.v[1]+'&f2='+s.v[2]+'&f3='+s.v[3]+'&f4='+s.v[4])}catch(e){}
busy=false},120);
fetch('/state').then(r=>r.json()).then(j=>{s=j;draw()});draw();
</script>)HTML";

// WiFiClient::write does not loop on a short write, so feed it in chunks
static void sendAll(WiFiClient& c, const char* data, size_t len) {
  while (len) {
    size_t n = c.write((const uint8_t*)data, len > 512 ? 512 : len);
    if (n == 0) return;
    data += n;
    len -= n;
  }
}

static void sendBody(WiFiClient& c, const char* type, const char* body, size_t len) {
  char head[128];
  int hn = snprintf(head, sizeof(head),
                    "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %u\r\n"
                    "Connection: close\r\n\r\n",
                    type, (unsigned)len);
  sendAll(c, head, hn);
  sendAll(c, body, len);
}

static void sendState(WiFiClient& c) {
  char body[64];
  int n = snprintf(body, sizeof(body), "{\"mode\":%u,\"v\":[%u,%u,%u,%u,%u]}", g_mode,
                   g_brightness, g_fader[0], g_fader[1], g_fader[2], g_fader[3]);
  sendBody(c, "application/json", body, n);
}

// -1 when the key is absent
static int param(const char* req, const char* key) {
  const char* p = strstr(req, key);
  if (!p) return -1;
  int v = atoi(p + strlen(key));
  return v < 0 ? 0 : (v > 255 ? 255 : v);
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
    if ((v = param(req, "bri=")) >= 0) g_brightness = v < MIN_BRIGHTNESS ? MIN_BRIGHTNESS : v;
    for (int i = 0; i < 4; i++) {
      char key[4] = {'f', (char)('1' + i), '=', 0};
      if ((v = param(req, key)) >= 0) g_fader[i] = v;
    }
    sendState(client);
  } else if (!strncmp(req, "GET /state", 10)) {
    sendState(client);
  } else if (!strncmp(req, "GET / ", 6)) {
    sendBody(client, "text/html", PAGE, sizeof(PAGE) - 1);
  } else {
    sendAll(client, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n", 45);
  }

  client.stop();
}
