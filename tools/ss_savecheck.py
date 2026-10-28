#!/usr/bin/env python3
"""Read-only SoulSilver .sav validator (never writes). Usage: ss_savecheck.py file.sav [...]
Checks the raw 512 KiB size, then every chunk footer of the two save slots (at 0x00000 and 0x40000):
footer = u32 save count, u32 chunk size (incl. 16-byte footer), u32 magic 0x20060623, u16 chunk id, u16 CRC16
(CRC-16 poly 0x1021, init 0xFFFF, MSB first, over the chunk bytes before the footer).
Reports which slot is newest, as the game picks the valid slot with the higher save count."""
import struct, sys
MAGIC = 0x20060623
def crc16(data):
    c = 0xFFFF
    for b in data:
        c ^= b << 8
        for _ in range(8):
            c = ((c << 1) ^ 0x1021) & 0xFFFF if c & 0x8000 else (c << 1) & 0xFFFF
    return c
def footers(d, base):
    out = []
    for off in range(base, base + 0x40000 - 15, 4):
        if struct.unpack_from('<I', d, off + 8)[0] == MAGIC:
            cnt, size, _, cid, crc = struct.unpack_from('<IIIHH', d, off)
            # extra chunks (Hall of Fame etc.) have a different footer that also holds the magic: only slot ids 0/1 are slot footers
            if cid > 1 or size < 16 or size > off + 16 - base or crc16(d[off + 16 - size:off]) != crc: continue
            out.append((off, cnt, size, cid, crc))
    return out
def check(path):
    d = open(path, 'rb').read()
    print(path, len(d), 'bytes', 'OK' if len(d) == 0x80000 else 'BAD SIZE (must be 524288)')
    if len(d) != 0x80000: return False
    best = {}
    for slot, base in enumerate((0, 0x40000)):
        ch = footers(d, base); good = []
        for off, cnt, size, cid, crc in ch:
            start = off + 16 - size
            ok = start >= base and size >= 16 and crc16(d[start:off]) == crc
            print('  slot%d chunk%d footer@%#x count=%d size=%d crc=%#06x %s' % (slot, cid, off, cnt, size, crc, 'VALID' if ok else 'INVALID'))
            if ok: good.append(cnt)
        ok = sorted(c[3] for c in ch) == [0, 1] and len(set(good)) == 1
        print('  slot%d: %s' % (slot, 'valid, save count %d' % good[0] if ok else 'NOT valid'))
        if ok: best[slot] = good[0]
    if best: print('  game loads slot', max(best, key=best.get))
    else: print('  no valid slot: the game would start a new game')
    return bool(best)
if __name__ == '__main__':
    sys.exit(0 if all([check(p) for p in sys.argv[1:]]) else 1)
