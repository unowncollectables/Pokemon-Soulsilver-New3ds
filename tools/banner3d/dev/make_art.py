#!/usr/bin/env python3
"""Remakes art/logo.png, art/portal.png and art/sparkle.png (committed; not needed to build the banner; needs numpy and
Pillow). logo.png is cut out of the repo's own "SoulSilver banner 1024.png" (keyed on the bright lettering, dilated to keep
the outline), resized to 224 px wide (1 texel = 1 screen pixel) and dithered to the 3DS's RGBA4 levels. portal.png is a
soft silver-blue glow with a gold ring (Lugia / Ho-Oh). sparkle.png is a 4-point star.

    python3 tools/banner3d/dev/make_art.py "SoulSilver banner 1024.png" tools/banner3d/art
"""
import sys, os
import numpy as np
from PIL import Image, ImageFilter


def dither(im):
    a = np.asarray(im.convert("RGBA")).astype(float)
    H, W = a.shape[:2]
    rgb = a[..., :3] * (a[..., 3:4] / 255.0); out = np.zeros_like(a)
    for c in range(3):
        ch = rgb[..., c].copy()
        for y in range(H):
            for x in range(W):
                old = ch[y, x]; new = np.round(old / 17) * 17; ch[y, x] = new; e = old - new
                if x + 1 < W: ch[y, x + 1] += e * 7 / 16
                if y + 1 < H:
                    if x > 0: ch[y + 1, x - 1] += e * 3 / 16
                    ch[y + 1, x] += e * 5 / 16
                    if x + 1 < W: ch[y + 1, x + 1] += e / 16
        out[..., c] = ch
    alq = np.round(a[..., 3] / 17) * 17; safe = np.where(alq > 0, alq, 1)
    out[..., :3] = np.clip(out[..., :3] * 255 / safe[..., None], 0, 255); out[..., 3] = alq
    return Image.fromarray(np.clip(out, 0, 255).astype(np.uint8), "RGBA")


def logo(src, dst, W=224):
    im = Image.open(src).convert("RGB")
    a = np.asarray(im).astype(float)
    bright = (a.max(axis=2) > 120)
    m = Image.fromarray((bright * 255).astype(np.uint8)).filter(ImageFilter.MaxFilter(9))
    m = m.filter(ImageFilter.GaussianBlur(1.5))
    rgba = im.convert("RGBA"); rgba.putalpha(m)
    rgba = rgba.crop(rgba.getchannel("A").point(lambda v: 255 if v > 8 else 0).getbbox())
    H = round(rgba.height * W / rgba.width)
    r = rgba.resize((W, H), Image.LANCZOS).filter(ImageFilter.UnsharpMask(radius=0.8, percent=60, threshold=2))
    dither(r).save(dst); print("logo", W, H)


def portal(dst, W=128, H=64):
    y, x = np.mgrid[0:H, 0:W]
    dx, dy = (x - W / 2 + .5) / (W / 2), (y - H / 2 + .5) / (H / 2)
    r = np.sqrt(dx * dx + dy * dy); ang = np.arctan2(dy, dx)
    rng = np.random.default_rng(7)
    noise = np.asarray(Image.fromarray((rng.random((8, 16)) * 255).astype(np.uint8)).resize((W, H), Image.BICUBIC)) / 255.0
    swirl = 0.5 + 0.5 * np.sin(3 * ang + 9 * r + 3 * noise)
    core = np.clip(1 - r, 0, 1) ** 1.4
    ring = np.exp(-((r - 0.78) / 0.07) ** 2)
    silver = np.array([150, 190, 255.0]); white = np.array([235, 245, 255.0]); gold = np.array([255, 205, 90.0])
    col = (silver * (1 - core[..., None] * 0.6) + white * (core[..., None] * 0.6)) * (0.7 + 0.3 * swirl[..., None])
    col = col * (1 - ring[..., None] * 0.6) + gold * ring[..., None] * 0.6
    alpha = np.clip((1 - r) * 1.6, 0, 1) ** 1.3 * (0.55 + 0.45 * swirl) * 255
    alpha = np.maximum(alpha, ring * 150 * (r < 1))
    Image.fromarray(np.dstack([col, alpha]).clip(0, 255).astype(np.uint8), "RGBA").save(dst); print("portal")


def sparkle(dst, S=16):
    y, x = np.mgrid[0:S, 0:S]
    dx, dy = np.abs(x - S / 2 + .5) / (S / 2), np.abs(y - S / 2 + .5) / (S / 2)
    a = np.clip(1 - (dx ** .5 + dy ** .5), 0, 1) ** 1.2 * 255
    rgb = np.dstack([np.full((S, S), 235.0), np.full((S, S), 245.0), np.full((S, S), 255.0)])
    dither(Image.fromarray(np.dstack([rgb, a]).clip(0, 255).astype(np.uint8), "RGBA")).save(dst); print("sparkle")


if __name__ == "__main__":
    os.makedirs(sys.argv[2], exist_ok=True)
    logo(sys.argv[1], os.path.join(sys.argv[2], "logo.png"))
    portal(os.path.join(sys.argv[2], "portal.png"))
    sparkle(os.path.join(sys.argv[2], "sparkle.png"))
