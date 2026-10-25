#!/usr/bin/env python3
# ss3ds: placeholder HOME Menu art, drawn here (original, no game assets, no image libraries needed):
# icon.png 48x48 (a silver disc), banner.png 256x128 (silver-to-night gradient), banner.wav 1 s of silence.
# usage: make_art.py OUTDIR
import struct, sys, wave, zlib
from pathlib import Path

def png(path, w, h, px):
    raw = b''.join(b'\0' + bytes(c for x in range(w) for c in px(x, y)) for y in range(h))
    chunk = lambda t, d: struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    path.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0)) +
                     chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))

def icon(x, y):
    d = ((x - 23.5) ** 2 + (y - 23.5) ** 2) ** 0.5
    if d > 22: return (32, 36, 62, 255)
    if d > 19: return (40, 46, 84, 255)
    v = int(235 - d * 4)
    return (v, v, min(255, v + 12), 255)

def banner(x, y):
    t = y / 127
    return (int(200 - 168 * t), int(204 - 166 * t), int(214 - 152 * t), 255)

out = Path(sys.argv[1]); out.mkdir(parents=True, exist_ok=True)
png(out / 'icon.png', 48, 48, icon)
png(out / 'banner.png', 256, 128, banner)
with wave.open(str(out / 'banner.wav'), 'wb') as w:
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(22050); w.writeframes(b'\0\0' * 22050)
