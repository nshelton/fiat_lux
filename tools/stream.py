#!/usr/bin/env python3
"""Stream frames to the fiat lux panel over UDP.

    python3 tools/stream.py 192.168.0.222

Frames are RGB bytes in raster order, top-left first -- the panel's serpentine
wiring is handled on the device. To drive it from your own code:

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    frame = bytearray(WIDTH * HEIGHT * 3)
    send(sock, (host, PORT), frame, seq)

Streaming overrides whatever animation is selected; the panel goes back to it
one second after the last packet.
"""
import math
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


def demo(frame, t):
    for y in range(HEIGHT):
        for x in range(WIDTH):
            v = math.sin(x / 3 + t) + math.sin(y / 2 - t) + math.sin((x + y) / 4 + t / 2)
            i = (y * WIDTH + x) * 3
            frame[i + 0] = int(127 + 127 * math.sin(v * 1.5))
            frame[i + 1] = int(127 + 127 * math.sin(v * 1.5 + 2.1))
            frame[i + 2] = int(127 + 127 * math.sin(v * 1.5 + 4.2))


def main():
    host = sys.argv[1] if len(sys.argv) > 1 else "fiatlux.local"
    addr = (socket.gethostbyname(host), PORT)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    frame = bytearray(NPIX * 3)

    print(f"streaming to {addr[0]}:{PORT} at {FPS} fps, ctrl-c to stop")
    seq = 0
    t0 = time.time()
    while True:
        demo(frame, time.time() - t0)
        send(sock, addr, frame, seq)
        seq += 1
        time.sleep(1 / FPS)


if __name__ == "__main__":
    main()
