#!/usr/bin/env bash
# find_rom.sh [--list] : print the path of the user's US SoulSilver .nds (first hit wins), or nothing.
# --list prints every place checked instead (for the build's warning). The ROM is only read, never copied.
# Order: SSROM / P3DS_SOULSILVER (env), the path the last ss3ds_build.sh was given (build/rom_path), the repo's roms/
# folder, Nathan's WSL convention ~/3ds/roms/, ~/roms/, a sibling roms/ folder, then the SD copy patch-builder installs.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cands=()
[ -n "${SSROM:-}" ] && cands+=("$SSROM")
[ -n "${P3DS_SOULSILVER:-}" ] && cands+=("$P3DS_SOULSILVER")
[ -f "$ROOT/build/rom_path" ] && cands+=("$(head -n1 "$ROOT/build/rom_path")")
for d in "$ROOT/roms" "$HOME/3ds/roms" "$HOME/roms" "$ROOT/../roms" "$ROOT/../3ds/roms"; do
  cands+=("$d/SoulSilver.nds" "$d/soulsilver.nds")
done
[ -n "${P3DS_SD:-}" ] && cands+=("$P3DS_SD/3ds/soulsilver3ds/soulsilver.nds")
if [ "${1:-}" = --list ]; then printf '%s\n' "${cands[@]}"; exit 0; fi
for c in "${cands[@]}"; do [ -f "$c" ] && { echo "$c"; exit 0; }; done
exit 0
