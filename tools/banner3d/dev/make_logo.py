#!/usr/bin/env python3
"""Remakes art/logo.png: a made logo laid out like the Platinum port's banner logo (small "POKEMON" over a big slanted
chrome title on a spiky burst, "NEW 3DS" tag at the lower right) in SoulSilver's silver and steel blue. Drawn from scratch at 4x
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


def outlined(mask, stops, out_r, out_col, in_col=None, bevel=0.0, gloss=0.0, shadow=0):
    """Fill + white inner line + dark outer outline, with an optional bevel (light from the top left), a top gloss band
    and a drop shadow, like the real title logos' raised lettering."""
    pad = out_r * 2 + shadow + 8
    base = Image.new("L", (mask.width + 2 * pad, mask.height + 2 * pad), 0)
    base.paste(mask, (pad, pad))
    layer = Image.new("RGBA", base.size, (0, 0, 0, 0))
    ring = grow(base, out_r)
    if shadow:
        sh = Image.new("L", base.size, 0); sh.paste(ring, (shadow, shadow)); sh = sh.filter(ImageFilter.GaussianBlur(shadow / 2))
        c = Image.new("RGBA", base.size, (0, 0, 10, 255)); c.putalpha(sh.point(lambda v: int(v * 0.7))); layer.alpha_composite(c)
    o = Image.new("RGBA", base.size, out_col + (255,)); o.putalpha(ring); layer.alpha_composite(o)
    if in_col:
        inner = grow(base, max(1, out_r // 3))
        i = Image.new("RGBA", base.size, in_col + (255,)); i.putalpha(inner); layer.alpha_composite(i)
    fill = fill_gradient(base, stops)
    if bevel or gloss:
        a = np.asarray(fill).astype(float)
        h = np.asarray(base.filter(ImageFilter.GaussianBlur(max(2, out_r * 0.9)))).astype(float) / 255.0
        gy, gx = np.gradient(h)
        light = np.clip((-gx - gy) * out_r * 3.0, -1, 1) * bevel          # lit from the top left
        rgb = a[..., :3]
        rgb = np.where(light[..., None] > 0, rgb + (255 - rgb) * light[..., None], rgb * (1 + light[..., None]))
        if gloss:
            yy = np.linspace(0, 1, base.height)[:, None]
            band = np.clip(1 - np.abs(yy - 0.28) / 0.16, 0, 1)[..., None] * gloss
            rgb = rgb + (255 - rgb) * band
        a[..., :3] = np.clip(rgb, 0, 255)
        fill = Image.fromarray(a.astype(np.uint8), "RGBA")
    layer.alpha_composite(fill)
    return layer


def feather(length, width):
    """One pointed feather, tip to the right, as an L mask with a lighter centre rib."""
    m = Image.new("L", (int(length), int(width)), 0)
    d = ImageDraw.Draw(m)
    pts = []
    n = 24
    for i in range(n + 1):
        t = i / n
        pts.append((t * length, width / 2 - (width / 2) * np.sin(np.pi * t ** 0.8) * (1 - 0.15 * t)))
    for i in range(n, -1, -1):
        t = i / n
        pts.append((t * length, width / 2 + (width / 2) * np.sin(np.pi * t ** 0.8) * (1 - 0.15 * t)))
    d.polygon(pts, fill=255)
    return m


def paste_feather(canvas, root, angle, length, width, col_a, col_b, rim):
    f = feather(length, width)
    f = f.resize((f.width, f.height), Image.LANCZOS)
    for grow_px, col in ((3 * S, rim), (0, None)):
        m = grow(f, grow_px) if grow_px else f
        if col:
            layer = Image.new("RGBA", m.size, col + (255,))
        else:
            layer = fill_gradient(Image.new("L", m.size, 255), [(0, col_a), (1, col_b)])
            layer.putalpha(f)
        layer.putalpha(m if col else f)
        pad = 8 * S
        big = Image.new("RGBA", (m.width + 2 * pad, m.height + 2 * pad), (0, 0, 0, 0)); big.paste(layer, (pad, pad))
        rot = big.rotate(angle, resample=Image.BICUBIC, center=(pad, pad + m.height / 2), expand=False)
        canvas.alpha_composite(rot, (int(root[0] - pad), int(root[1] - pad - m.height / 2)))


def burst(canvas):
    d = ImageDraw.Draw(canvas)
    navy, steel, silver, ice = (10, 20, 56), (70, 110, 180), (190, 205, 230), (235, 245, 255)
    # navy body behind the lettering with a silver rim
    pts_r = [(0.50, 0.30), (0.64, 0.28), (0.78, 0.26), (0.88, 0.34), (0.92, 0.50), (0.88, 0.66), (0.78, 0.76), (0.64, 0.84), (0.50, 0.86)]
    pts = [(x * W, y * H) for x, y in pts_r] + [((1 - x) * W, y * H) for x, y in reversed(pts_r)]
    body = Image.new("L", (W, H), 0); ImageDraw.Draw(body).polygon(pts, fill=255)
    body = body.filter(ImageFilter.GaussianBlur(3 * S)).point(lambda v: 255 if v > 128 else 0)
    for grow_px, col in ((10, (14, 30, 90)), (7, silver), (4, ice), (0, navy)):
        m = grow(body, grow_px) if grow_px else body
        c = Image.new("RGBA", (W, H), col + (255,)); c.putalpha(m); canvas.alpha_composite(c)
    # steel-blue glow in the middle of the body
    glow = Image.new("L", (W, H), 0)
    ImageDraw.Draw(glow).ellipse((W * 0.14, H * 0.24, W * 0.86, H * 0.90), fill=170)
    glow = glow.filter(ImageFilter.GaussianBlur(18 * S))
    g = Image.new("RGBA", (W, H), (50, 100, 200, 255)); g.putalpha(ImageChops.multiply(glow, body)); canvas.alpha_composite(g)
    # silver-blue wings (Lugia): feathers fan out from behind the title to each side, darker ones behind the lighter
    wings = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    root = (W / 2 + W * 0.27, H * 0.52)
    for ang, ln, wd in ((-40, 0.2, 14), (-20, 0.235, 16), (0, 0.25, 17), (20, 0.235, 16), (40, 0.2, 14)):
        paste_feather(wings, root, -ang, W * ln, wd * S, (150, 185, 235), (240, 248, 255), (12, 28, 90))
    canvas.alpha_composite(wings)
    canvas.alpha_composite(wings.transpose(Image.FLIP_LEFT_RIGHT))
    # ocean swell lines under the lettering
    for k, y0 in enumerate((0.76, 0.82)):
        pts2 = [(x * W, (y0 + 0.025 * np.sin(x * 20 + k)) * H) for x in np.linspace(0.12, 0.88, 60)]
        d.line(pts2, fill=(150, 190, 245, 200), width=2 * S)
    # four-point glints
    for (fx, fy, fr) in ((0.06, 0.20, 9), (0.95, 0.70, 8), (0.52, 0.05, 7), (0.30, 0.95, 6)):
        cx, cy2, r = fx * W, fy * H, fr * S
        d.polygon([(cx, cy2 - r * 1.6), (cx + r * 0.3, cy2 - r * 0.3), (cx + r * 1.6, cy2), (cx + r * 0.3, cy2 + r * 0.3),
                   (cx, cy2 + r * 1.6), (cx - r * 0.3, cy2 + r * 0.3), (cx - r * 1.6, cy2), (cx - r * 0.3, cy2 - r * 0.3)], fill=(235, 245, 255, 255))


def main(dst, preview=None):
    canvas = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    burst(canvas)
    big = shear(text_mask("SOULSILVER", 60 * S, spacing=1 * S), -0.20)
    big = big.resize((int(W * 0.82), int(big.height * W * 0.82 / big.width)), Image.LANCZOS)
    chrome = [(0, (255, 255, 255)), (0.42, (205, 218, 238)), (0.50, (150, 168, 200)), (0.58, (190, 205, 228)), (1, (245, 248, 255))]
    stamp(canvas, outlined(big, chrome, 4 * S, (8, 14, 46), (120, 170, 240), bevel=0.55, gloss=0.45, shadow=3 * S), W / 2, H * 0.27)
    pk = shear(text_mask("POKÉMON", 22 * S, spacing=2 * S), -0.12)
    pk = pk.resize((int(W * 0.34), int(pk.height * W * 0.34 / pk.width)), Image.LANCZOS)
    stamp(canvas, outlined(pk, [(0, (250, 252, 255)), (0.5, (190, 215, 245)), (1, (120, 165, 225))], 3 * S, (14, 30, 100), (255, 255, 255), bevel=0.5, gloss=0.35, shadow=2 * S), W / 2, H * 0.03)
    tag = shear(text_mask("NEW 3DS", 18 * S, spacing=1 * S), -0.20)
    tag = tag.resize((int(W * 0.26), int(tag.height * W * 0.26 / tag.width)), Image.LANCZOS)
    stamp(canvas, outlined(tag, [(0, (150, 220, 255)), (0.5, (40, 130, 220)), (1, (15, 60, 150))], 2 * S, (255, 255, 255), (8, 20, 70), bevel=0.4, gloss=0.3, shadow=2 * S), W * 0.72, H * 0.70)
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
