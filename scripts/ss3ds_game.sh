#!/usr/bin/env bash
# ss3ds_game.sh ROM : pspoke's scripts/soulsilver.sh up to (not including) the PSP renderer and EBOOT link.
# Copied into pspoke/scripts by bootstrap.sh and run there with toolchain-3ds/bin first on PATH, so every object
# it makes is a devkitARM (ARM11, hard-float) object. Keep it in step with pspoke's soulsilver.sh when bumping
# upstream.lock. The 3DS link (platform layer + these objects) is done by soulsilver3ds/full/Makefile.
source "$(dirname "$0")/phases.sh"
ROM="$1"
phase_base; phase_generated; phase_services; phase_libraries
# ss3ds (0138): compile-network.py is staged once and the network step runs inside the one-shot "libraries" phase, so a
# change to its flags (0137: -DSDK_BUILD_LINUX, the socket backend for online trading) never reached an already staged tree.
# Patch the staged copy and rebuild only libnetwork-probe.a; make relinks it into the game.
NW="$T/native-probe/compile-network.py"
if [ -f "$NW" ] && ! grep -q SDK_BUILD_LINUX "$NW"; then
  sed -i "s/'-DSDK_BUILD_PSP',/'-DSDK_BUILD_PSP','-DSDK_BUILD_LINUX',/" "$NW"
  grep -q SDK_BUILD_LINUX "$NW" || die "ss3ds: could not add SDK_BUILD_LINUX to the staged compile-network.py"
  log "ss3ds: staged compile-network.py had no SDK_BUILD_LINUX; rebuilding libnetwork-probe.a"
  step network-linux bash -c "cd '$T/native-probe' && python3 compile-network.py"
fi
C="$T/soulsilver-native-core"; P="$C/nitromain-perf"
# ss3ds: the staged tree keeps the prepare scripts copied when it was first staged, so a pspoke-patch that changes
# them (0053/0054 raw DISPCNT rewrite) never ran there and the wild battle crash shipped twice. Re-sync them and, when
# one changed, redo prepare and the game compile (one long rebuild, then incremental again).
for f in prepare.py finish_headers.py final_compat.py; do
  if [ -d "$C" ] && ! cmp -s "$ROOT/port/soulsilver-native-core/$f" "$C/$f"; then
    cp -f "$ROOT/port/soulsilver-native-core/$f" "$C/$f"
    rm -f "$WORK/.stamp-soulsilver-prepare" "$WORK/.stamp-soulsilver-game"
    log "ss3ds: staged $f was stale, re-synced; re-running SoulSilver prepare + game compile"
  fi
done

# ss3ds: seeds.json (maploader translator roots) is staged once too; patch 0127 adds the comm/Union Room roots, so re-sync it
# and redo the game compile (the translator re-emits overworld_native.c).
for f in seeds.json seed-files.json port.py native-provided.json; do
  S="$T/soulsilver-native-play/maploader/$f"
  if [ -d "$C" ] && [ -f "$S" ] && ! cmp -s "$ROOT/port/soulsilver-native-play/maploader/$f" "$S"; then
    cp -f "$ROOT/port/soulsilver-native-play/maploader/$f" "$S"
    rm -f "$WORK/.stamp-soulsilver-game"
    log "ss3ds: staged maploader/$f was stale, re-synced; re-running the SoulSilver game compile"
  fi
done

if ! done_ soulsilver-prepare; then
  log "Preparing the HeartGold/SoulSilver decompilation"
  "$ROOT/scripts/fetch.sh" pokeheartgold-slop pokeheartgold
  mkdir -p "$T/soulsilver-research" "$T/soulsilver-native-audit"
  rsync -a --exclude .git "$U/pokeheartgold-slop/" "$T/soulsilver-research/pokeheartgold-slop/"
  rsync -a --exclude .git "$U/pokeheartgold/" "$T/soulsilver-native-audit/pokeheartgold/"
  fill_rom SOULSILVER_ROM "$ROM"
  step ss-member-maps  bash -c "cd '$T/soulsilver-native-assets' && python3 audit_members.py"
  step ss-prepare      bash -c "cd '$C' && python3 prepare.py && python3 finish_headers.py && python3 final_compat.py"
  step ss-patch        bash -c "cd '$C' && for d in src include game-include qol; do patch -p1 -s < '$ROOT/patches/soulsilver/'\$d.patch; done"
  mark soulsilver-prepare
fi

if ! done_ soulsilver-game; then
  log "Compiling SoulSilver for the 3DS (devkitARM)"
  step ss-core         bash -c "cd '$C' && python3 crossprobe.py && python3 archive.py"
  step ss-aliasing-fix bash -c "cd '$C' && python3 -c \"import json,subprocess;subprocess.run(json.load(open('compile-command.json'))+['-fno-strict-aliasing','-c','src/overlay_02_02248728.c','-o','objects/overlay_02_02248728.o'],check=True)\" && psp-ar r libsoulsilver-c.a objects/overlay_02_02248728.o"
  step ss-data         bash -c "cd '$T/soulsilver-native-data' && python3 build_data.py"
  step ss-overlays     bash -c "cd '$T/soulsilver-native-overlays' && python3 build_registry.py"
  step ss-codegen      bash -c "cd '$T/soulsilver-native-codegen' && python3 library.py"
  step ss-maploader    bash -c "cd '$T/soulsilver-native-play/maploader' && python3 port.py --compile"
  step ss-movement     bash -c "cd '$T/soulsilver-native-player-movement' && make movement.o terrain.o control.o && psp-ar rcs libss-player-movement.a movement.o terrain.o control.o"
  step ss-menu-sprites bash -c "cd '$T/soulsilver-native-menu-sprites' && make progress.o graphics.o && psp-ar rcs libss-menu-sprites.a progress.o graphics.o"
  step ss-window       bash -c "cd '$T/soulsilver-native-window' && python3 port.py"
  step ss-sound        bash -c "cd '$T/soulsilver-native-sound-helpers' && make helpers.o && psp-ar rcs libss-sound-helpers.a helpers.o"
  step ss-fade         make -C "$T/soulsilver-native-assets/fade-port/wipe-candidate" libss-native-fade.a
  step ss-wfc          bash -c "cd '$T/soulsilver-native-islands' && make wfc_startup.o && psp-ar rcs libss-wfc-startup.a wfc_startup.o"
  step ss-particles    bash -c "cd '$T/soulsilver-native-particles' && python3 build.py"
  step ss-billboards   bash -c "cd '$T/soulsilver-native-billboards' && python3 build.py && make bootstrap.o"
  step ss-overlay-ids  bash -c "cd '$T/native-audio-app/overlays' && [ -f include/nitro/fs/native_overlay_ids.h ] || python3 generate.py --headers-only"
  step ss-services     make -C "$T/native-audio-app" owner_lock.o backup.o audio_bank.o audio_loader.o audio_seq.o audio_exchannel.o audio_channel.o offline.o gx_dma.o threads.o alarms.o mi_memory.o mi_dma.o backing.o graphics_registers.o power.o mic_pm.o os_sync.o sdl_threads.o ctrdg_absent.o
  step ss-sound-engine make -C "$T/native-sound-audio" audio_backend.o audio_engine.o sas_out.o
  for o in owner_lock backup audio_bank audio_loader audio_seq audio_exchannel audio_channel offline gx_dma threads alarms mi_memory mi_dma backing graphics_registers power mic_pm os_sync sdl_threads ctrdg_absent; do cp -f "$T/native-audio-app/$o.o" "$P/"; done
  cp -f "$T/native-sound-audio/audio_backend.o" "$T/native-sound-audio/audio_engine.o" "$T/native-sound-audio/sas_out.o" "$P/"
  cp -f "$T/soulsilver-native-billboards/bootstrap.o" "$P/billboard_bootstrap.o"
  cp -f "$T/native-audio-app/libsdk-filtered.a.base" "$P/libsdk-filtered-ss.a"
  step ss-sdk          psp-objcopy --weaken-symbol=MTX_Copy33To43_ --weaken-symbol=MTX_Copy33To44_ --weaken-symbol=MTX_Copy43To44_ --weaken-symbol=MTX_Scale33_ --weaken-symbol=MTX_Scale43_ --weaken-symbol=MTX_Scale44_ --weaken-symbol=MTX_Transpose33_ --weaken-symbol=MTX_Transpose43_ --weaken-symbol=MTX_Transpose44_ "$P/libsdk-filtered-ss.a"
  step ss-overlay-obj  bash -c "cd '$P' && make overlay.o"
  step ss-cull-vectors bash -c "cd '$P' && python3 generate_culling_vectors.py"
  mark soulsilver-game
fi

# Fixes carried over from the Platinum 3DS port. A tree staged before a pspoke-patch was added keeps the old copy
# of the file, so the files those patches change are re-synced here; make then rebuilds only what changed.
for f in native-sound-audio/sas_out.c soulsilver-native-assets/render-fastcompare/g3_handler.cpp \
         soulsilver-native-assets/render-fastcompare/frontend.cpp soulsilver-native-core/nitromain-perf/romfs.c \
         soulsilver-native-core/nitromain-perf/platform.c soulsilver-native-core/nitromain-perf/frame.c \
         soulsilver-native-core/nitromain-perf/missing.c native-backup-probe/backup.c \
         native-audio-app/services/mi_dma.c; do   # missing.c: patches 0118/0125 drop traps, a stale staged copy duplicates them at link
  cmp -s "$ROOT/port/$f" "$T/$f" || { cp -f "$ROOT/port/$f" "$T/$f"; log "ss3ds: staged $f was stale, re-synced"; }
done
# The staged Makefile has its @WORK@ paths filled in, so it gets the 0015 link wrap in place instead of a copy.
grep -q -- '--wrap=G3X_GetVectorMtx' "$P/Makefile" || sed -i 's/ -Wl,--wrap=G3X_GetClipMtx / -Wl,--wrap=G3X_GetClipMtx -Wl,--wrap=G3X_GetVectorMtx /' "$P/Makefile"
# platinum3ds run 43 sound cut-off: libntr's SND_SetChannelPan did "reg = ~(...)" instead of "reg &= ~(...)", wiping
# the whole SOUNDxCNT on every pan change (format bits, enable, volume lost = voices cut off).
SNDC="$T/native-graphics/libntr/libraries/sim7/src/snd/snd_channel.c"
grep -q 'reg = ~(0b1111111 << 16);' "$SNDC" && sed -i 's/reg = ~(0b1111111 << 16);/reg \&= ~(0b1111111 << 16); \/* ss3ds: was "reg = ~(...)" (platinum3ds run 43) *\//' "$SNDC"
step ss-fix-sound    bash -c "make -C '$T/native-sound-audio' sas_out.o && make -C '$T/native-audio-sound' audio_channel.o && make -C '$T/native-audio-app' audio_channel.o"
cp -f "$T/native-sound-audio/sas_out.o" "$T/native-audio-app/audio_channel.o" "$P/"
# patch 0070 (save fsync) changed backup.c: rebuild backup.o from the re-synced file (make skips it when current).
step ss-backup       bash -c "make -C '$T/native-audio-app' backup.o"
cp -f "$T/native-audio-app/backup.o" "$P/"
# ss3ds (0140): mi_dma.c is staged once too (0139's NULL-source DMA guard never reached an incremental build). Same
# treatment: re-synced above, rebuilt here, and the object copied next to the others so the link picks it up.
step ss-mi-dma       bash -c "make -B -C '$T/native-audio-app' mi_dma.o"
cp -f "$T/native-audio-app/mi_dma.o" "$P/"

# ss3ds: the ROM-side "instant text" is now the Settings app's FAST TEXT option (patches/soulsilver/textspeed.patch),
# off by default, so the compile-time switch stays off. textspeed.patch is applied here (not in the prepare block)
# so a tree staged by an older build gets it too; the marker keeps it from applying twice.
export PSPOKE_QOL_INSTANT_TEXT="${PSPOKE_QOL_INSTANT_TEXT:-0}"
qol_header
(cd "$C" && grep -q RenderFontFast src/text.c || patch -p1 -s < "$ROOT/patches/soulsilver/textspeed.patch")
# ss3ds gameplay toggles (patches/soulsilver/gameplay.patch): reusable TMs, HM-free field moves, no wild encounters and
# the repel prompt switch, each read at run time from the Settings app. Applied here for the same reason as above.
(cd "$C" && grep -q SS3DS_HmFreeSlot src/script_pokemon_util.c || patch -p1 -s < "$ROOT/patches/soulsilver/gameplay.patch")
step ss-qol          bash -c "cd '$C' && for f in 'src/text.c text.o none' 'src/render_text.c render_text.o none' 'src/item.c item.o none' 'src/pokemon.c pokemon.o none' 'src/script_manager.c script_manager.o none' 'src/scrcmd_c.c scrcmd_c.o none' 'src/move.c move.o none' 'src/party_menu_items.c party_menu_items.o none' 'src/script_pokemon_util.c script_pokemon_util.o none' 'src/scrcmd_party.c scrcmd_party.o none' 'src/field/scrcmd_message.c field__scrcmd_message.o 1' 'src/field/encounter_check.c field__encounter_check.o 2'; do bash rebuild_game_object.sh \$f || exit 1; done"
# The melonDS-derived 2D/3D cores the 3DS platform layer links against (pspoke's render-gu2d/g3_backend are PSP GE
# code and are replaced by soulsilver3ds/platform).
step ss-melon-2d     make -C "$T/native-render-opt" native_gpu.o GPU2D_Soft.o
step ss-window-adapt make -C "$T/soulsilver-native-assets/render-fastcompare" window_adapter.o g3_handler.o frontend.o
cp -f "$T/soulsilver-native-assets/render-fastcompare/window_adapter.o" "$P/window_adapter.o"   # as pspoke soulsilver.sh does
# pspoke's SoulSilver app objects (its OBJS minus the PSP-only ones soulsilver3ds/full/Makefile drops).
SSOBJS=$(sed -n 's/^OBJS = //p' "$P/Makefile" | tr ' ' '\n' | grep -vxE 'main\.o|osk\.o|naming_osk_ss\.o|timer3_psp\.o' | tr '\n' ' ')
step ss-app-objs     bash -c "make -C '$P' PSP_NATIVE_PROBE_FRAMES=0 PSP_NATIVE_PLAY=1 $SSOBJS"
log "SoulSilver game objects ready under $C (next: make -C soulsilver3ds/full)"

# ss3ds feature hooks: scripts/ss3ds_hooks.d/NNNN_name.sh, sourced in number order (they see $C, $P, $T, $ROOT and step).
# Each one edits the staged decomp idempotently and rebuilds only the objects it touched.
for h in "$ROOT"/scripts/ss3ds_hooks.d/*.sh; do [ -f "$h" ] && . "$h"; done
