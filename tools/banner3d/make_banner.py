#!/usr/bin/env python3
"""The SoulSilver HOME Menu 3D banner. The portal, sparkles and logo are this repo's own art (art/); with --rom the title
screen's Lugia (model, textures and wing beat) is read from your own ROM at build time and flies in the middle. The
banner made then contains the game's model, so it is never committed or shared. Without --rom, or if Lugia cannot be
read from the ROM, the banner is the art-only one. Layout and the CGFX fixes follow the Platinum 3DS port's
tools/banner3d (GabeSud0/pokeplatinum-3ds, GPL-3.0); see README.md.

    python3 tools/banner3d/make_banner.py [--rom SoulSilver.nds] --bannertool BANNERTOOL --chime banner.wav -o build/cia/banner3d.bnr

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
TITLE_NARC = "a/0/4/6"                       # the title screen's models (pokeheartgold: title_screen graphics)
LUGIA_MODEL_NAME = "lugia01_10t"             # the NSBMD's own name; its wing-beat NSBCA is the next member


def lugia_from_rom(rom_path):
    """(nsbmd, nsbca) of the title-screen Lugia from a SoulSilver ROM; ValueError if it is not where expected."""
    import nitrofs
    import narc
    data = open(rom_path, "rb").read()
    if data[0xC:0x10] != b"IPGE":
        raise ValueError(f"{rom_path} is not a US SoulSilver ROM (game code {data[0xC:0x10]!r}, wanted b'IPGE')")
    entry = next((e for e in nitrofs.parse_nitrofs(data) if e["path"] == TITLE_NARC), None)
    if entry is None:
        raise ValueError(f"{TITLE_NARC} isn't in this ROM")
    archive = data[entry["offset"]:entry["offset"] + entry["size"]]
    index = 0
    while True:
        try:
            member = narc.member(archive, index)
        except ValueError:
            break                                          # past the last member
        if member[:4] == b"BMD0" and LUGIA_MODEL_NAME.encode() in member[:0x100]:
            nsbca = narc.member(archive, index + 1)
            if nsbca[:4] != b"BCA0":
                raise ValueError("the title Lugia's animation isn't the next member")
            return member, nsbca
        index += 1
    raise ValueError(f"no {LUGIA_MODEL_NAME} model in {TITLE_NARC}")


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
    ap.add_argument("--rom", help="your US SoulSilver ROM (.nds): the title-screen Lugia goes in the banner")
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

    lugia = None
    if args.rom:
        try:
            lugia = lugia_from_rom(args.rom)
            print(f"Lugia: {len(lugia[0])} + {len(lugia[1])} bytes from {args.rom}")
        except (OSError, ValueError) as ex:
            print(f"(No Lugia in the banner: {ex}. Using the art-only banner.)")

    fetch_pycgfx(args.pycgfx, to_cgfx.PYCGFX_COMMIT)
    with tempfile.TemporaryDirectory() as tmp:
        work = args.keep or tmp
        try:
            gltf = scene.build(os.path.join(HERE, "art"), work, sparkles=not args.no_sparkles, lugia=lugia)
            cgfx = to_cgfx.convert(args.pycgfx, gltf)
        except Exception as ex:                      # a ROM that parses oddly must not cost the banner
            if lugia is None:
                raise
            print(f"(Lugia could not be made into a banner: {ex!r}. Using the art-only banner.)")
            lugia = None
            gltf = scene.build(os.path.join(HERE, "art"), work, sparkles=not args.no_sparkles)
            cgfx = to_cgfx.convert(args.pycgfx, gltf)
        cgfx_path = os.path.join(work, "banner.cgfx")
        with open(cgfx_path, "wb") as f:
            f.write(cgfx)
        digest = hashlib.sha1(cgfx).hexdigest()
        os.makedirs(os.path.dirname(os.path.abspath(args.output)), exist_ok=True)
        subprocess.run([args.bannertool, "makebanner", "-ci", cgfx_path, "-a", args.chime, "-o", args.output],
                       check=True, stdout=subprocess.DEVNULL)
    print(f"Made {args.output} (3D banner{' with Lugia' if lugia else ', art only'}, CGFX {len(cgfx)} bytes, sha1 {digest[:12]}, UNTESTED on a real HOME Menu)")


if __name__ == "__main__":
    main()
