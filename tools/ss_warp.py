#!/usr/bin/env python3
"""Box-only: copy a SoulSilver save and move the player (newest slot's Location at +0x1234), fixing the chunk CRC."""
import struct, sys
sys.path.insert(0, '/workspace/soulsilver3ds/tools')
from ss_savecheck import crc16, footers
src, dst, m, x, z = sys.argv[1], sys.argv[2], *map(lambda v: int(v, 0), sys.argv[3:6])
warp = int(sys.argv[6], 0) if len(sys.argv) > 6 else -1
d = bytearray(open(src, 'rb').read())
best = None
for base in (0, 0x40000):
    ch = footers(d, base)
    if ch and (best is None or ch[0][1] > best[1]): best = (base, ch[0][1], ch)
base, _, ch = best
ox, oz = struct.unpack_from('<ii', d, base + 0x123c)
struct.pack_into('<5i', d, base + 0x1234, m, warp, x, z, 1)
# saved map objects (player and follower, stride 0x50 near +0x2300): their u16 x / z (and the pair at +0x23cc) move too
for o in range(base + 0x2000, base + 0x2800, 2):
    a = struct.unpack_from('<H', d, o)[0]
    if a == ox:
        for dz in (4, 2):
            zz = struct.unpack_from('<H', d, o + dz)[0]
            if abs(zz - oz) <= 2:
                struct.pack_into('<H', d, o, x); struct.pack_into('<H', d, o + dz, z + (zz - oz)); print('  moved object pos at', hex(o)); break
for off, cnt, size, cid, crc in ch:
    start = off + 16 - size
    if start <= base + 0x1234 < off: struct.pack_into('<H', d, off + 14, crc16(d[start:off]))
open(dst, 'wb').write(d)
print('warped', dst, 'slot base', hex(base), (m, x, z))
