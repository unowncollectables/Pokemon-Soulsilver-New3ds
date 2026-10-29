# ss3ds full-game link: pspoke's SoulSilver objects (scripts/ss3ds_build.sh, built by the psp-* -> devkitARM
# wrappers) + the 3DS platform layer (../platform) -> soulsilver3ds.3dsx.
# Adapted from the Platinum 3DS port's platinum3ds/full/Makefile. DRAFT: not linked yet; expect undefined symbols
# from Platinum-only hooks in ../platform (P3DS_MapId, follower, settings items) on the first try.
#
# The game's object, archive and --wrap lists are read from pspoke's own SoulSilver Makefile (nitromain-perf), so a
# pspoke bump keeps them in step. PSP-only pieces are filtered out below and replaced by ../platform.
ROOT   := $(abspath ../..)
W      ?= $(ROOT)/pspoke/.work/tree/test_out
DKP    ?= $(or $(DEVKITPRO),/opt/devkitpro)
TC     := $(ROOT)/toolchain-3ds/bin
CC     := $(TC)/psp-gcc
CXX    := $(TC)/psp-g++
DKA    := $(DKP)/devkitARM/bin/arm-none-eabi-
P      := $(W)/soulsilver-native-core/nitromain-perf
F      := $(W)/soulsilver-native-assets/render-fastcompare
SSMK   := $(P)/Makefile
# pspoke's PSP program: its main, the firmware keyboard (we keep the DS naming screen), the PSP timer and the
# PSP GE renderer archive are replaced by ../platform (sdl_threads.o: sdl_ctru.c has the SDL threads).
COMMA  := ,
PSPONLY:= main.o osk.o naming_osk_ss.o timer3_psp.o sdl_threads.o
SSOBJS := $(filter-out $(PSPONLY),$(shell sed -n 's/^OBJS = //p' $(SSMK) 2>/dev/null))
SSLIBS := $(filter-out libss-render.a -lpsp% -lstdc++ -lm,$(shell sed -n 's/^LIBS = //p' $(SSMK) 2>/dev/null))
# --wrap=OverlayManager_New belongs to naming_osk_ss.o (the PSP keyboard naming screen), which we drop.
SSWRAP := $(filter-out -Wl$(COMMA)--wrap=OverlayManager_New,$(filter -Wl$(COMMA)--wrap=%,$(shell sed -n 's/^LDFLAGS = //p' $(SSMK) 2>/dev/null)))
SSVER  := $(shell sed -n 's/^#define SS3DS_VERSION "\(.*\)"/\1/p' ../platform/ss3ds_version.h)
# CIA title version shown by FBI = 0.<run>.0, taken from the "run N" in ss3ds_version.h
CIA_MINOR ?= $(shell sed -n 's/^#define SS3DS_VERSION "run \([0-9]*\).*/\1/p' ../platform/ss3ds_version.h)
DEFS   := -O2 -DSDK_PORT -DSDK_X86 -DSDK_TS -DSDK_4M -DSDK_FINALROM -DNNS_FINALROM -DSDK_VERSION_MAJOR=4 -DSDL_MAIN_HANDLED -DMELONDS_PSP -DPSP_NATIVE_RGBA_OUTPUT=1 -DSS3DS -ffunction-sections -fdata-sections
INC    := -I$(W)/native-probe/backing/include -I$(W)/native-graphics/alias-proof/include -I$(W)/native-probe/generated -I$(W)/native-graphics/libntr/include -I$(W)/../melon -I$(W)/native-render-opt -I$(W)/soulsilver-native-core/include -I$(W)/soulsilver-native-core/game-include
PLAT   := ../platform
POBJS  := build/main3ds.o build/logring.o build/psp_ctru.o build/sdl_ctru.o build/stubs3ds.o build/render3ds.o build/g3soft.o build/gpuscale.o build/blit_shbin.o build/g3gpu.o build/g3_shbin.o build/gpucomp.o build/par.o build/romcache3ds.o build/speedcheck.o build/p3ds_gamefont.o build/psp_ss_ctru.o build/ss3ds_glue.o build/wm_uds.o build/uds_ctru.o build/ss3ds_net.o build/widecomp.o build/dcskip.o build/hunt.o build/ribbons.o build/tracker.o build/celebrate.o build/ss3ds_dwcstub.o
# wireless (docs/WIRELESS.md M3): wm_uds.o is the WM API (from the Platinum port, unchanged). It replaces the SDK's WM
# members of libsdk-filtered-ss.a, which forward to an ARM7 that does not exist here. wireless=0 = pspoke's
# wm_absent.c behaviour (a radio with nobody in range), as in Platinum. Its callbacks are pumped from
# __wrap_PSPNativeFrameComplete (main3ds.c), because SoulSilver's frame.c has no PSPNativeWMPump call.
# melonDS 2D core + SoulSilver's 3D handler/frontend (the parts of libss-render.a that are not PSP GE code).
RENDER := $(W)/native-render-opt/native_gpu.o $(W)/native-render-opt/GPU2D_Soft.o $(F)/g3_handler.o $(F)/frontend.o
# Platinum 3DS link wraps that are about the platform, not the game (game-specific ones get re-checked on SoulSilver).
P3WRAP := -Wl,--wrap=abort,--wrap=OverlayManager_New,--wrap=OS_WaitVBlankIntr,--wrap=OS_WaitIrq,--wrap=MI_WaitDma,--wrap=printf,--wrap=PSPNativeSasSnapshot,--wrap=puts,--wrap=PSPNativeFatal,--wrap=CTRDG_Init,--wrap=PSPNativeMemLog,--wrap=GXx_SetMasterBrightness_,--wrap=GXx_GetMasterBrightness_,--wrap=MapHeader_GetAreaDataBank,--wrap=sub_02025488,--wrap=sub_020254FC,--wrap=PSPNativeFrameComplete,--wrap=sceKernelDcacheWritebackInvalidateRange
ARCH   := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

all: soulsilver3ds.3dsx
check:
	@test -f $(SSMK) || { echo "Run scripts/ss3ds_build.sh first (no $(SSMK))"; exit 1; }
	@echo "game objects: $(words $(SSOBJS)), archives: $(words $(SSLIBS)), game wraps: $(words $(SSWRAP))"
build/%.o: $(PLAT)/%.c
	@mkdir -p build; $(CC) $(DEFS) $(INC) -c $< -o $@
build/main3ds.o build/stubs3ds.o: DEFS += -DPLATINUM3DS_APP
build/%.o: $(PLAT)/%.cpp
	@mkdir -p build; $(CXX) $(DEFS) $(INC) -std=gnu++17 -fno-exceptions -fno-rtti -fpermissive -c $< -o $@
build/sdl_ctru.o build/psp_ctru.o build/psp_ss_ctru.o build/ss3ds_glue.o build/uds_ctru.o build/ss3ds_net.o: INC :=
# libctru/citro3d ABI uses short enums; game-facing code uses int enums (pspoke/DS data layout).
build/sdl_ctru.o build/psp_ctru.o build/psp_ss_ctru.o build/gpuscale.o build/uds_ctru.o build/ss3ds_net.o: DEFS += -fshort-enums
build/blit_shbin.h build/blit_shbin.s: $(PLAT)/shaders/blit.v.pica
	@mkdir -p build; $(DKP)/tools/bin/picasso -o build/blit.shbin $< && cd build && $(DKP)/tools/bin/bin2s -H blit_shbin.h blit.shbin > blit_shbin.s
build/g3_shbin.h build/g3_shbin.s: $(PLAT)/shaders/g3.v.pica
	@mkdir -p build; $(DKP)/tools/bin/picasso -o build/g3.shbin $< && cd build && $(DKP)/tools/bin/bin2s -H g3_shbin.h g3.shbin > g3_shbin.s
build/%_shbin.o: build/%_shbin.s
	$(DKA)as $< -o $@
build/gpuscale.o: $(PLAT)/gpuscale.c build/blit_shbin.h
	$(CC) $(DEFS) -Ibuild -I$(DKP)/libctru/include -c $< -o $@
# As in the Platinum Makefile: the two GPU files see only libctru and the generated shader headers.
build/gpucomp.o: $(PLAT)/gpucomp.c build/blit_shbin.h
	$(CC) $(DEFS) -fshort-enums -Ibuild -I$(DKP)/libctru/include -c $< -o $@
build/g3gpu.o: $(PLAT)/g3gpu.c build/g3_shbin.h
	$(CC) $(DEFS) -fshort-enums -Ibuild -I$(DKP)/libctru/include -c $< -o $@
build/soulsilver3ds.ld: mkld_ss.py $(W)/soulsilver-native-overlays/ranges.h
	@mkdir -p build; python3 mkld_ss.py $(DKP)/devkitARM/arm-none-eabi/lib/3dsx.ld $(W)/soulsilver-native-overlays/ranges.h $@
build/p3ds.specs:
	@mkdir -p build; sed 's#-T 3dsx.ld%s#-T $(CURDIR)/build/soulsilver3ds.ld#' $(DKP)/devkitARM/arm-none-eabi/lib/3dsx.specs > $@
soulsilver3ds.elf: check $(POBJS) build/soulsilver3ds.ld build/p3ds.specs
	cd $(P) && $(DKA)gcc -specs=$(CURDIR)/build/p3ds.specs -B$(DKP)/devkitARM/arm-none-eabi/lib/ $(ARCH) -Wl,--gc-sections $(SSWRAP) $(P3WRAP) -Wl,--no-enum-size-warning -Wl,-Map,$(CURDIR)/build/full.map \
	  $(addprefix $(CURDIR)/,$(POBJS)) $(SSOBJS) $(RENDER) $(SSLIBS) -L$(DKP)/libctru/lib -lcitro3d -lctru -lstdc++ -lm -o $(CURDIR)/$@
soulsilver3ds.smdh: ../platform/ss3ds_version.h cia/art/icon.png
	@# run 3: depends on the version (was stuck at the first build's string) and uses the committed SoulSilver icon art
	$(DKP)/tools/bin/smdhtool --create "SoulSilver 3DS ($(SSVER))" "native New 3DS port" "local build" cia/art/icon.png $@
soulsilver3ds.3dsx: soulsilver3ds.elf soulsilver3ds.smdh
	$(DKP)/tools/bin/3dsxtool $< $@ --smdh=soulsilver3ds.smdh
clean:
	rm -rf build soulsilver3ds.*
.PHONY: all check clean

# HOME Menu version (same ELF and the same sdmc:/3ds/soulsilver3ds/ files). rsf from the Platinum 3DS port (itself
# from GabeSud0/pokeplatinum-3ds): 124 MB memory mode, 804 MHz, L2 cache and the third core on a New 3DS.
# makerom and bannertool are built from source once by tools/build_cia_tools.sh.
CIA_TOOLS ?= $(ROOT)/build/cia-tools
BT     := $(CIA_TOOLS)/bin/bannertool
MAKEROM:= $(CIA_TOOLS)/bin/makerom
# ROM search: SSROM=..., then tools/find_rom.sh (build/rom_path, roms/, ~/3ds/roms/, ~/roms/, the SD copy). Missing = LOUD warning below.
SSROM  ?= $(shell bash $(ROOT)/tools/find_rom.sh)
# HOME Menu banner. DEFAULT = 3d (Nathan asked for it, 2026-10-26; no ROM or any failure -> flat). The
# first 3D Lugia banner froze the HOME Menu when the cursor reached the icon (2026-10-26). The 3D banner is now built like the
# Platinum port's (portal, Lugia, logo; no sparkles; Lugia cut to Platinum's 1000 triangles; same CGFX structure, checked by
# tools/banner3d/cgfx_fingerprint.py against reference/platinum_banner.json). Any limit or structure mismatch -> flat.
#   BANNER=flat       banner.png + the original chime (the PR #17 banner that runs on Nathan's HOME Menu)
#   BANNER=3d         (default) Platinum-style 3D banner with the title-screen Lugia from the ROM (needs the ROM, else flat)
#   BANNER=3d-art     3D banner with portal + logo only (no ROM art)
#   BANNER=3d-lugia   same as 3d (kept so old command lines work)
BANNER ?= 3d
cia: soulsilver3ds.cia
$(MAKEROM) $(BT):
	bash $(ROOT)/tools/build_cia_tools.sh $(CIA_TOOLS)
soulsilver3ds.cia: soulsilver3ds.elf $(ROOT)/tools/home_menu_art.py cia/soulsilver3ds.rsf cia/make_art.py cia/art/icon.png cia/art/banner.png $(ROOT)/tools/banner3d/make_banner.py $(ROOT)/tools/banner3d/scene.py $(ROOT)/tools/banner3d/lugia_parts.py $(ROOT)/tools/banner3d/decimate.py $(ROOT)/tools/banner3d/to_cgfx.py $(ROOT)/tools/banner3d/cgfx_fingerprint.py $(ROOT)/tools/banner3d/reference/platinum_banner.json | $(MAKEROM) $(BT)
	python3 cia/make_art.py build/cia
	@if [ -f "$(SSROM)" ]; then echo "[ROM] using $(SSROM) for the icon (and the Lugia banner if BANNER=3d)"; else \
	  echo "[ROM] ************************************************************************"; \
	  echo "[ROM] WARNING: original icon (and no Lugia for BANNER=3d): ROM not found at '$(SSROM)'"; \
	  echo "[ROM] Checked:"; bash $(ROOT)/tools/find_rom.sh --list | sed 's/^/[ROM]   /'; \
	  echo "[ROM] Fix: make cia SSROM=/path/to/SoulSilver.nds   (or put it at ~/3ds/roms/SoulSilver.nds)"; \
	  echo "[ROM] ************************************************************************"; fi
	@# ss3ds run 1: like the Platinum port, the HOME Menu icon is your ROM's own DS icon (Lugia), made here in build/ and
	@# never committed; no ROM (SSROM) -> the committed original icon in cia/art/. The banner stays the original art.
	if [ -f "$(SSROM)" ]; then python3 $(ROOT)/tools/home_menu_art.py "$(SSROM)" build/cia/rom && cp build/cia/rom/icon.png build/cia/icon.png && echo 'icon: from the ROM'; else echo 'icon: original art (no ROM at $(SSROM))'; fi
	@# Banner: flat unless BANNER=3d / 3d-lugia was asked for; any 3D failure or limit breach falls back to flat. Which one
	@# was built is printed in a [BANNER] line (and again after the build by the "[BANNER] RESULT" line).
	@rm -f build/cia/banner3d.bnr
	@case "$(BANNER)" in flat|3d|3d-art|3d-lugia) ;; *) echo "[BANNER] ERROR: BANNER=$(BANNER) is not flat, 3d, 3d-art or 3d-lugia"; exit 1;; esac
	@echo "[BANNER] ******** requested banner: $(BANNER) (default 3d; no ROM or a failure falls back to flat) ********"
	@if [ "$(BANNER)" = 3d-art ]; then python3 $(ROOT)/tools/banner3d/make_banner.py --bannertool $(BT) --chime build/cia/banner3d.wav \
	    -o build/cia/banner3d.bnr || { rm -f build/cia/banner3d.bnr; echo "[BANNER] 3D banner FAILED: using the FLAT one."; }; fi
	@if [ "$(BANNER)" = 3d ] || [ "$(BANNER)" = 3d-lugia ]; then if [ -f "$(SSROM)" ]; then python3 $(ROOT)/tools/banner3d/make_banner.py --rom "$(SSROM)" --bannertool $(BT) --chime build/cia/banner3d.wav \
	    -o build/cia/banner3d.bnr || { rm -f build/cia/banner3d.bnr; echo "[BANNER] 3D Lugia banner FAILED: using the FLAT one."; }; else echo "[BANNER] BANNER=3d needs the ROM ($(SSROM)): using the FLAT one."; fi; fi
	@if [ -f build/cia/banner3d.bnr ]; then cp build/cia/banner3d.bnr build/cia/banner.bnr; echo "[BANNER] RESULT: 3D ($(BANNER)) banner.bnr $$(stat -c %s build/cia/banner.bnr) bytes. UNTESTED on a real HOME Menu: if HOME freezes on the icon, delete the title with FBI and rebuild with BANNER=flat."; \
	else $(BT) makebanner -i build/cia/banner.png -a build/cia/banner.wav -o build/cia/banner.bnr; echo "[BANNER] RESULT: FLAT banner.bnr $$(stat -c %s build/cia/banner.bnr) bytes (the known-good PR #17 banner)"; fi
	$(BT) makesmdh -s "SoulSilver 3DS" -l "Pokemon SoulSilver native New 3DS port" -p "$(SSVER)" -i build/cia/icon.png -o build/cia/icon.icn
	$(MAKEROM) -f cia -o $@ -elf soulsilver3ds.elf -rsf cia/soulsilver3ds.rsf -icon build/cia/icon.icn -banner build/cia/banner.bnr -exefslogo -target t -major 0 -minor $(CIA_MINOR) -micro 0
.PHONY: cia

# the version string is compiled into these two; rebuild them when it changes
build/render3ds.o build/main3ds.o: ../platform/ss3ds_version.h
