#!/usr/bin/env bash
# bootstrap.sh : fetch pspoke at the commit in upstream.lock into ./pspoke and apply pspoke-patches/*.patch.
#   bash scripts/bootstrap.sh          fetch (if needed) and apply
#   bash scripts/bootstrap.sh --clean  delete ./pspoke (and its .work/.cache) and start over
# pspoke downloads its own pinned decompilations and SDK sources during the build; nothing here is game data.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"; P="$ROOT/pspoke"
lock(){ sed -n "s/^$1=//p" "$ROOT/upstream.lock"; }
URL=$(lock url); COMMIT=$(lock commit)
[ "${1:-}" = --clean ] && { rm -rf "$P"; echo "removed $P"; }
if [ ! -d "$P/.git" ]; then
  git clone --filter=blob:none "$URL" "$P"
fi
cd "$P"
if git rev-parse -q --verify ss3ds >/dev/null; then
  echo "pspoke already bootstrapped (branch ss3ds). Use --clean to redo it."; exit 0
fi
git fetch -q origin "$COMMIT" 2>/dev/null || true
git checkout -q -b ss3ds "$COMMIT"
git -c user.name=ss3ds -c user.email=ss3ds@localhost am -q "$ROOT"/pspoke-patches/*.patch
cp -f "$ROOT/scripts/ss3ds_game.sh" scripts/ss3ds_game.sh
echo "pspoke ready at $P ($(git log -1 --format=%h) + $(ls "$ROOT"/pspoke-patches/*.patch | wc -l) patches)"
