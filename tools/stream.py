#!/usr/bin/env python3
"""Stream frames to the fiat lux panel over UDP.

    python3 tools/stream.py fiatlux.local            # default pattern
    python3 tools/stream.py fiatlux.local ripple     # pick one
    python3 tools/stream.py --list

Frames are RGB bytes in raster order, top-left first -- the panel's serpentine
wiring is handled on the device. A pattern is any function that fills the
frame buffer; to drive the panel from your own code:

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    frame = bytearray(WIDTH * HEIGHT * 3)
    send(sock, (host, PORT), frame, seq)

Streaming overrides whatever animation is selected; the panel goes back to it
one second after the last packet.
"""
import math
import random
import socket
import sys
import time

WIDTH, HEIGHT = 32, 16
PORT = 8001
CHUNK = 128  # pixels per packet; must be <= 255, the header's count is one byte
FPS = 30

NPIX = WIDTH * HEIGHT


def send(sock, addr, frame, seq):
    for start in range(0, NPIX, CHUNK):
        count = min(CHUNK, NPIX - start)
        last = start + count >= NPIX
        head = bytes((ord('F'), ord('L'), seq & 0xFF, start >> 8, start & 0xFF, count, last))
        sock.sendto(head + bytes(frame[start * 3:(start + count) * 3]), addr)


# ---- patterns: fill frame[] with RGB triplets, index (y * WIDTH + x) * 3 ----

def plasma(frame, t):
    for y in range(HEIGHT):
        for x in range(WIDTH):
            v = math.sin(x / 3 + t) + math.sin(y / 2 - t) + math.sin((x + y) / 4 + t / 2)
            i = (y * WIDTH + x) * 3
            frame[i + 0] = int(127 + 127 * math.sin(v * 1.5))
            frame[i + 1] = int(127 + 127 * math.sin(v * 1.5 + 2.1))
            frame[i + 2] = int(127 + 127 * math.sin(v * 1.5 + 4.2))


def rainbow(frame, t):
    for y in range(HEIGHT):
        for x in range(WIDTH):
            h = (x + y) / 24 - t / 2
            i = (y * WIDTH + x) * 3
            for c in range(3):
                frame[i + c] = int(127 + 127 * math.sin(2 * math.pi * (h + c / 3)))


def ripple(frame, t):
    cx, cy = WIDTH / 2 - 0.5, HEIGHT / 2 - 0.5
    for y in range(HEIGHT):
        for x in range(WIDTH):
            d = math.hypot(x - cx, (y - cy) * 1.6)
            v = math.sin(d / 1.5 - t * 3) / (1 + d / 6)
            i = (y * WIDTH + x) * 3
            frame[i + 0] = int(max(0, v) * 255)
            frame[i + 1] = int(abs(v) * 90)
            frame[i + 2] = int(max(0, -v) * 255)


HEAT = [0] * NPIX


def fire(frame, _t):
    for x in range(WIDTH):  # seed the bottom row
        HEAT[(HEIGHT - 1) * WIDTH + x] = random.randint(150, 255)
    for y in range(HEIGHT - 1):  # cool and drift upward
        for x in range(WIDTH):
            b = (y + 1) * WIDTH
            avg = (HEAT[b + x] * 2 + HEAT[b + max(0, x - 1)] + HEAT[b + min(WIDTH - 1, x + 1)]) // 4
            HEAT[y * WIDTH + x] = max(0, avg - 14)
    for p, h in enumerate(HEAT):
        i = p * 3
        frame[i + 0] = min(255, h * 2)
        frame[i + 1] = max(0, min(255, (h - 96) * 2))
        frame[i + 2] = max(0, min(255, (h - 192) * 4))


def bounce(frame, t):
    for i in range(NPIX * 3):  # fade to black rather than clearing, leaves a trail
        frame[i] = frame[i] * 3 // 4
    x = int((WIDTH - 1) * (0.5 + 0.5 * math.sin(t * 1.7)))
    y = int((HEIGHT - 1) * (0.5 + 0.5 * math.sin(t * 2.3)))
    i = (y * WIDTH + x) * 3
    frame[i + 0], frame[i + 1], frame[i + 2] = 255, 200, 60


PATTERNS = {"plasma": plasma, "rainbow": rainbow, "ripple": ripple,
            "fire": fire, "bounce": bounce}


def main():
    if "--list" in sys.argv:
        print(" ".join(PATTERNS))
        return

    host = sys.argv[1] if len(sys.argv) > 1 else "fiatlux.local"
    name = sys.argv[2] if len(sys.argv) > 2 else "plasma"
    if name not in PATTERNS:
        sys.exit(f"unknown pattern {name!r}; have: {' '.join(PATTERNS)}")
    pattern = PATTERNS[name]

    addr = (socket.gethostbyname(host), PORT)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    frame = bytearray(NPIX * 3)

    print(f"streaming {name} to {addr[0]}:{PORT} at {FPS} fps, ctrl-c to stop")
    seq = 0
    t0 = time.time()
    while True:
        pattern(frame, time.time() - t0)
        send(sock, addr, frame, seq)
        seq += 1
        time.sleep(1 / FPS)


if __name__ == "__main__":
    main()
