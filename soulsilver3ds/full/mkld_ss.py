# ss3ds: devkitARM's 3dsx.ld plus SoulSilver's per-overlay data/bss output sections (.ssov.N.data / .ssov.N.bss,
# made by pspoke's build_registry.py), each with the ss_ov_N_{data,bss}_{start,end} symbols overlay.c reads.
# pspoke puts the same sections into the PSP linker script; Platinum's mkld.py does this for Platinum's overlays.
# usage: mkld_ss.py 3dsx.ld ranges.h out.ld
import re, sys
ld, ranges, out = sys.argv[1:4]
s = open(ld).read()
n = sorted({int(x) for x in re.findall(r'ss_ov_(\d+)_data_start', open(ranges).read())})
if not n: sys.exit('mkld_ss: no overlays in ' + ranges)
sec = []
for kind in ('data', 'bss'):
    for i in n:
        p = f'ss_ov_{i}_{kind}'
        sec.append(f'\t.ssov.{i}.{kind} ' + ('(NOLOAD) ' if kind == 'bss' else '') +
                   f': ALIGN(16) {{ {p}_start = .; KEEP(*(.ssov.{i}.{kind}*)) {p}_end = .; }}')
i = s.index('\t.tdata : ALIGN(4)')
open(out, 'w').write(s[:i] + '\n'.join(sec) + '\n' + s[i:])
print(f'mkld_ss: {len(n)} overlays')
