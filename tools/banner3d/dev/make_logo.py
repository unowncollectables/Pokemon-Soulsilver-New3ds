#!/usr/bin/env python3
"""Remakes art/logo.png: a made logo laid out like the Platinum port's banner logo (small "POKEMON" over a big slanted
chrome title on a spiky burst, "NEW 3DS" tag at the lower right) in SoulSilver's silver and gold. Drawn from scratch at 4x
with system fonts (no game or third-party art), downsized to exactly 224 px (1 texel = 1 screen pixel) and dithered to the
3DS's RGBA4 levels like dev/make_art.py. Not needed to build the banner (the PNG is committed); needs numpy and Pillow.

    python3 tools/banner3d/dev/make_logo.py tools/banner3d/art/logo.png [PREVIEW.png]
"""
import sys, os
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter, ImageChops

sys.path.insert(0, os.path.dirname(__file__))
from make_art import dither

S = 4
W, H = 224 * S, 84 * S
FONTS = ["/usr/share/fonts/opentype/inter/Inter-Black.otf", "/usr/share/fonts/opentype/inter/Inter-ExtraBold.otf",
         "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"]


def font(size):
    for f in FONTS:
        if os.path.exists(f):
            return ImageFont.truetype(f, size)
    raise SystemExit("no bold font found")


def text_mask(txt, size, spacing=0):
    f = font(size)
    widths = [f.getlength(c) for c in txt]
    w = int(sum(widths) + spacing * (len(txt) - 1)) + 8
    m = Image.new("L", (w, int(size * 1.4)), 0)
    d = ImageDraw.Draw(m)
    x = 4
    for c, cw in zip(txt, widths):
        d.text((x, 0), c, font=f, fill=255)
        x += cw + spacing
    return m.crop(m.getbbox())


def shear(m, k):
    w, h = m.size
    pad = int(h * abs(k)) + 2
    return m.transform((w + pad, h), Image.AFFINE, (1, k, -pad if k > 0 else 0, 0, 1, 0), Image.BICUBIC)


def fill_gradient(mask, stops):
    w, h = mask.size
    t = np.linspace(0, 1, h)[:, None]
    xs = [s[0] for s in stops]
    img = np.stack([np.interp(t[:, 0], xs, [s[1][c] for s in stops]) for c in range(3)], axis=-1)
    img = np.repeat(img[:, None, :], w, axis=1).astype(np.uint8)
    out = Image.fromarray(img, "RGB").convert("RGBA")
    out.putalpha(mask)
    return out


def grow(mask, r):
    return mask.filter(ImageFilter.MaxFilter(r * 2 + 1)) if r > 0 else mask


def stamp(canvas, layer, cx, top):
    canvas.alpha_composite(layer, (int(cx - layer.width / 2), int(top)))


def outlined(mask, stops, out_r, out_col, in_col=None):
    pad = out_r + 6
    base = Image.new("L", (mask.width + 2 * pad, mask.height + 2 * pad), 0)
    base.paste(mask, (pad, pad))
    layer = Image.new("RGBA", base.size, (0, 0, 0, 0))
    ring = grow(base, out_r)
    layer.alpha_composite(Image.new("RGBA", base.size, out_col + (255,)).convert("RGBA"), (0, 0)) if False else None
    o = Image.new("RGBA", base.size, out_col + (255,)); o.putalpha(ring); layer.alpha_composite(o)
    if in_col:
        inner = grow(base, max(1, out_r // 3))
        i = Image.new("RGBA", base.size, in_col + (255,)); i.putalpha(inner); layer.alpha_composite(i)
    layer.alpha_composite(fill_gradient(base, stops))
    return layer


def burst(canvas):
    d = ImageDraw.Draw(canvas)
    cy = H * 0.60
    # spiky wing burst: dark navy body, gold rim, silver inner rim
    pts_r = [(0.50, 0.30), (0.62, 0.30), (0.74, 0.20), (0.86, 0.06), (0.93, 0.04), (0.95, 0.16), (1.00, 0.20), (0.96, 0.38),
             (1.00, 0.46), (0.95, 0.58), (0.99, 0.72), (0.90, 0.82), (0.74, 0.80), (0.62, 0.90), (0.50, 0.88)]
    pts = [(x * W, y * H) for x, y in pts_r] + [((1 - x) * W, y * H) for x, y in reversed(pts_r)]
    for grow_px, col in ((14, (245, 190, 60)), (8, (255, 236, 160)), (0, (14, 26, 64))):
        layer = Image.new("L", (W, H), 0)
        ImageDraw.Draw(layer).polygon(pts, fill=255)
        layer = grow(layer, grow_px) if grow_px else layer
        c = Image.new("RGBA", (W, H), col + (255,)); c.putalpha(layer); canvas.alpha_composite(c)
    # gold and silver streaks like Platinum's side flares
    for sgn in (1, -1):
        for (x0, y0, x1, y1, col) in ((0.30, 0.34, 0.02, 0.10, (255, 205, 80)), (0.30, 0.50, 0.01, 0.48, (200, 220, 255)),
                                       (0.34, 0.72, 0.06, 0.90, (255, 205, 80))):
            ax = (0.5 + sgn * (0.5 - x0)) * W; bx = (0.5 + sgn * (0.5 - x1)) * W
            d.line([(ax, y0 * H), (bx, y1 * H)], fill=col + (255,), width=3 * S)


def main(dst, preview=None):
    canvas = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    burst(canvas)
    big = shear(text_mask("SOULSILVER", 60 * S, spacing=1 * S), -0.20)
    big = big.resize((int(W * 0.88), int(big.height * W * 0.88 / big.width)), Image.LANCZOS)
    chrome = [(0, (255, 255, 255)), (0.42, (205, 218, 238)), (0.50, (120, 140, 175)), (0.58, (190, 205, 228)), (1, (245, 248, 255))]
    stamp(canvas, outlined(big, chrome, 3 * S, (10, 18, 50), (255, 255, 255)), W / 2, H * 0.27)
    pk = shear(text_mask("POKÉMON", 22 * S, spacing=2 * S), -0.12)
    pk = pk.resize((int(W * 0.34), int(pk.height * W * 0.34 / pk.width)), Image.LANCZOS)
    stamp(canvas, outlined(pk, [(0, (255, 240, 120)), (1, (255, 190, 30))], 2 * S, (20, 50, 150), (255, 255, 255)), W / 2, H * 0.03)
    tag = shear(text_mask("NEW 3DS", 18 * S, spacing=1 * S), -0.20)
    tag = tag.resize((int(W * 0.26), int(tag.height * W * 0.26 / tag.width)), Image.LANCZOS)
    stamp(canvas, outlined(tag, [(0, (255, 120, 110)), (1, (205, 20, 40))], 2 * S, (255, 255, 255), (60, 8, 20)), W * 0.72, H * 0.70)
    box = canvas.getchannel("A").point(lambda a: 255 if a > 8 else 0).getbbox()
    canvas = canvas.crop(box)
    h = round(canvas.height * 224 / canvas.width)
    small = canvas.resize((224, h), Image.LANCZOS).filter(ImageFilter.UnsharpMask(radius=0.8, percent=60, threshold=2))
    out = dither(small)
    out.save(dst); print("logo", out.size)
    if preview:
        bg = Image.new("RGBA", out.size, (40, 40, 60, 255)); bg.alpha_composite(out)
        bg.resize((out.width * 3, out.height * 3), Image.NEAREST).save(preview)


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else None)
