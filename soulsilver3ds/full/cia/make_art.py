#!/usr/bin/env python3
# ss3ds: HOME Menu art for the .cia (original, no game assets, no image libraries needed).
# icon.png 48x48 and banner.png 256x128 are committed in art/ (banner scaled from the repo's "SoulSilver banner
# 1024.png", icon drawn to match it); banner.wav is synthesized here: an original ~2.9 s jingle (rising silver-bell phrase over a soft pad, a gold chime on the
# last note), stereo 32728 Hz like the Platinum port's chime. No game audio.
# usage: make_art.py OUTDIR
import math, shutil, struct, sys, wave
from pathlib import Path

art = Path(__file__).resolve().parent / 'art'
out = Path(sys.argv[1]); out.mkdir(parents=True, exist_ok=True)
for name in ('icon.png', 'banner.png'):
    shutil.copyfile(art / name, out / name)

RATE, LEN = 32728, 2.9
N = int(RATE * LEN)
L, R = [0.0] * N, [0.0] * N

def bell(start, f, amp=1.0, pan=0.5, decay=2.6, gold=False):
    s0 = int(start * RATE)
    for i in range(N - s0):
        t = i / RATE
        env = min(1, t / 0.004) * math.exp(-t * decay)
        v = math.sin(2 * math.pi * f * t) + 0.35 * math.sin(2 * math.pi * f * 2.76 * t) * math.exp(-t * 6)
        if gold:
            v += 0.5 * math.sin(2 * math.pi * f * 1.5 * t) * math.exp(-t * 3)   # warm fifth: the Ho-Oh side
        v *= env * amp
        L[s0 + i] += v * (1 - pan); R[s0 + i] += v * pan

def pad(f, amp):
    for i in range(N):
        t = i / RATE
        env = math.sin(math.pi * min(1, t / LEN)) ** 1.5
        v = amp * env * (math.sin(2 * math.pi * f * t) + 0.4 * math.sin(2 * math.pi * f * 2 * t))
        L[i] += v; R[i] += v

# silver rise: E5 G#5 B5 E6 (E major pentatonic reach), a held B6, then a gold E7 chime with its fifth
for start, f, pan in ((0.00, 659.3, 0.35), (0.18, 830.6, 0.45), (0.36, 987.8, 0.55), (0.54, 1318.5, 0.65),
                      (0.90, 1975.5, 0.4)):
    bell(start, f, 0.9, pan)
bell(1.25, 2637.0, 1.0, 0.5, 2.0, gold=True)
bell(1.25, 1318.5, 0.6, 0.5, 1.6)
pad(164.8, 0.12); pad(246.9, 0.08)          # E3 + B3 pad
# echo for space
D = int(RATE * 0.23)
for i in range(N - 1, D - 1, -1):
    L[i] += 0.3 * R[i - D]; R[i] += 0.3 * L[i - D]
peak = max(max(abs(v) for v in L), max(abs(v) for v in R)) or 1
fade = int(RATE * 0.35)
pcm = bytearray()
for n in range(N):
    g = 0.5 * 32767 / peak * min(1, (N - n) / fade)
    pcm += struct.pack('<hh', int(L[n] * g), int(R[n] * g))
with wave.open(str(out / 'banner.wav'), 'wb') as w:
    w.setnchannels(2); w.setsampwidth(2); w.setframerate(RATE); w.writeframes(bytes(pcm))
