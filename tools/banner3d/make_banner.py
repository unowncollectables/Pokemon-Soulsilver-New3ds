#!/usr/bin/env python3
"""The SoulSilver HOME Menu 3D banner, built only from this repo's own art (art/): no ROM and no game assets needed.
Layout and the CGFX fixes follow the Platinum 3DS port's tools/banner3d (GabeSud0/pokeplatinum-3ds, GPL-3.0), minus
the ROM-extracted Giratina; see README.md.

    python3 tools/banner3d/make_banner.py --bannertool BANNERTOOL --chime banner.wav -o build/cia/banner3d.bnr

The scene is turned into CGFX by skyfloogle/pycgfx, fetched once at a pinned commit into build/pycgfx (needs git),
with the fixes in to_cgfx.py. Needs Python 3.12 or later; standard library only.
"""
import argparse
import hashlib
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

PYCGFX_REPO = "https://github.com/skyfloogle/pycgfx.git"


def fetch_pycgfx(dest, commit):
    if os.path.isfile(os.path.join(dest, "main.py")):
        head = subprocess.run(["git", "-C", dest, "rev-parse", "HEAD"], capture_output=True, text=True).stdout.strip()
        if head == commit:
            return
    print(f"Fetching pycgfx ({commit[:7]}) into {dest}")
    os.makedirs(dest, exist_ok=True)
    git = lambda *a: subprocess.run(["git", "-C", dest, *a], check=True, stdout=subprocess.DEVNULL)
    if not os.path.isdir(os.path.join(dest, ".git")):
        git("init", "-q")
    git("fetch", "-q", "--depth", "1", PYCGFX_REPO, commit)
    git("checkout", "-q", "--force", "FETCH_HEAD")


def main():
    if sys.version_info < (3, 12):
        sys.exit("The 3D banner needs Python 3.12 or later (pycgfx uses it).")
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--fetch-pycgfx-only", action="store_true", help="just fetch pycgfx (for the tests)")
    ap.add_argument("--no-sparkles", action="store_true", help="portal + logo only (nothing animated)")
    ap.add_argument("--bannertool", help="bannertool (tools/build_cia_tools.sh)")
    ap.add_argument("--chime", help="the banner's sound (.wav, at most 3 seconds)")
    ap.add_argument("--pycgfx", default=os.path.join(ROOT, "build", "pycgfx"), help="where pycgfx is fetched")
    ap.add_argument("-o", "--output", help="the .bnr to write")
    ap.add_argument("--keep", help="keep the glTF scene and CGFX in this folder")
    args = ap.parse_args()

    import scene
    import to_cgfx

    if args.fetch_pycgfx_only:
        fetch_pycgfx(args.pycgfx, to_cgfx.PYCGFX_COMMIT)
        return
    if not (args.bannertool and args.chime and args.output):
        ap.error("needs --bannertool, --chime and -o")

    fetch_pycgfx(args.pycgfx, to_cgfx.PYCGFX_COMMIT)
    with tempfile.TemporaryDirectory() as tmp:
        work = args.keep or tmp
        gltf = scene.build(os.path.join(HERE, "art"), work, sparkles=not args.no_sparkles)
        cgfx = to_cgfx.convert(args.pycgfx, gltf)
        cgfx_path = os.path.join(work, "banner.cgfx")
        with open(cgfx_path, "wb") as f:
            f.write(cgfx)
        digest = hashlib.sha1(cgfx).hexdigest()
        os.makedirs(os.path.dirname(os.path.abspath(args.output)), exist_ok=True)
        subprocess.run([args.bannertool, "makebanner", "-ci", cgfx_path, "-a", args.chime, "-o", args.output],
                       check=True, stdout=subprocess.DEVNULL)
    print(f"Made {args.output} (3D banner, CGFX {len(cgfx)} bytes, sha1 {digest[:12]}, UNTESTED on a real HOME Menu)")


if __name__ == "__main__":
    main()
