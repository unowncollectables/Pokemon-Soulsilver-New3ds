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
  # Already bootstrapped: apply only the patches added since (matched by subject line).
  git checkout -q ss3ds; n=0; have=$(git log --format=%s)
  for p in "$ROOT"/pspoke-patches/*.patch; do
    subj=$(sed -n 's/^Subject: \[PATCH[^]]*\] //p' "$p" | head -1)
    grep -qF "$subj" <<<"$have" && continue
    # Older builds copied files into pspoke untracked that a later patch now adds (0017: scripts/ss3ds_game.sh);
    # git am refuses to overwrite them, so move any such leftover aside first.
    for f in $(git apply --summary "$p" | sed -n 's/^ create mode [0-9]* //p'); do
      if [ -e "$f" ] && ! git ls-files --error-unmatch -- "$f" >/dev/null 2>&1; then
        mv -f "$f" "$f.pre-patch"; echo "moved untracked $f aside to $f.pre-patch (patch $(basename "$p") adds it)"
      fi
    done
    git -c user.name=ss3ds -c user.email=ss3ds@localhost am -q "$p"; n=$((n+1)); echo "applied $(basename "$p")"
  done
  echo "pspoke up to date ($n new patches). Files already staged in pspoke/.work need the same change by hand."; exit 0
fi
git fetch -q origin "$COMMIT" 2>/dev/null || true
git checkout -q -b ss3ds "$COMMIT"
git -c user.name=ss3ds -c user.email=ss3ds@localhost am -q "$ROOT"/pspoke-patches/*.patch
echo "pspoke ready at $P ($(git log -1 --format=%h) + $(ls "$ROOT"/pspoke-patches/*.patch | wc -l) patches)"
