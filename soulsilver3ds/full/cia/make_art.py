#!/usr/bin/env python3
# ss3ds: HOME Menu art for the .cia (original, no game assets, no image libraries needed).
# icon.png 48x48 and banner.png 256x128 are committed in art/ (banner scaled from the repo's "SoulSilver banner
# 1024.png", icon drawn to match it); banner.wav is synthesized here: a short silver-bell arpeggio, about 2.5 s.
# usage: make_art.py OUTDIR
import math, shutil, struct, sys, wave
from pathlib import Path

art = Path(__file__).resolve().parent / 'art'
out = Path(sys.argv[1]); out.mkdir(parents=True, exist_ok=True)
for name in ('icon.png', 'banner.png'):
    shutil.copyfile(art / name, out / name)

RATE, LEN = 22050, 2.6
notes = [(0.00, 1318.5), (0.14, 1568.0), (0.28, 1975.5), (0.42, 2637.0), (0.70, 1975.5), (0.84, 2637.0)]  # E6 G6 B6 E7
buf = [0.0] * int(RATE * LEN)
for start, f in notes:
    s0 = int(start * RATE)
    for i in range(len(buf) - s0):
        t = i / RATE
        env = min(1, t / 0.004) * math.exp(-t * 3.2)
        buf[s0 + i] += env * (math.sin(2 * math.pi * f * t) + 0.3 * math.sin(2 * math.pi * f * 2.76 * t) * math.exp(-t * 6))
peak = max(abs(v) for v in buf) or 1
fade = int(RATE * 0.3)
pcm = b''.join(struct.pack('<h', int(v / peak * 0.5 * 32767 * min(1, (len(buf) - n) / fade))) for n, v in enumerate(buf))
with wave.open(str(out / 'banner.wav'), 'wb') as w:
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(RATE); w.writeframes(pcm)
