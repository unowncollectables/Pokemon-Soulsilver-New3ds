#!/usr/bin/env bash
# ss3ds_build.sh /path/to/SoulSilver.nds : full from-scratch game compile with the 3DS toolchain.
#   needs: devkitPro (3ds-dev), python3 (+ pip install ndspy), git, make, patch, rsync, curl
# The ROM is only read in place; it is never copied into this repository.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ROM="$(cd "$(dirname "${1:?usage: ss3ds_build.sh /path/to/SoulSilver.nds}")" && pwd)/$(basename "$1")"
[ -f "$ROM" ] || { echo "no ROM at $ROM" >&2; exit 1; }
export DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}" DEVKITARM="${DEVKITARM:-${DEVKITPRO:-/opt/devkitpro}/devkitARM}"
[ -x "$DEVKITARM/bin/arm-none-eabi-gcc" ] || { echo "devkitARM not found at $DEVKITARM (install devkitPro 3ds-dev)" >&2; exit 1; }
WANT=$(sed -n 's/^soulsilver_sha1=//p' "$ROOT/upstream.lock")
GOT=$( (command -v sha1sum >/dev/null && sha1sum "$ROM" || shasum -a 1 "$ROM") | cut -d' ' -f1)
[ "$GOT" = "$WANT" ] || { echo "error: this ROM is not the SoulSilver (US) dump the port is tested with." >&2; echo "  ROM SHA1:      $GOT" >&2; echo "  expected SHA1: $WANT" >&2; echo "  Build stopped. Use a clean SoulSilver (US) .nds." >&2; exit 1; }
mkdir -p "$ROOT/build" && echo "$ROM" > "$ROOT/build/rom_path"   # lets `make cia` find the same ROM (tools/find_rom.sh)
bash "$ROOT/scripts/bootstrap.sh"   # fetches pspoke once, later only applies newly added pspoke-patches
# PSPDEV points pspoke at our stand-in SDK so it never downloads the PSP toolchain.
export PSPDEV="$ROOT/toolchain-3ds" PATH="$ROOT/toolchain-3ds/bin:$DEVKITARM/bin:$DEVKITPRO/tools/bin:$PATH"
cd "$ROOT/pspoke" && bash scripts/ss3ds_game.sh "$ROM"
