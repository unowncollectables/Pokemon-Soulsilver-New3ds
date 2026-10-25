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
# PSP GE renderer archive are replaced by ../platform.
COMMA  := ,
PSPONLY:= main.o osk.o naming_osk_ss.o timer3_psp.o
SSOBJS := $(filter-out $(PSPONLY),$(shell sed -n 's/^OBJS = //p' $(SSMK) 2>/dev/null))
SSLIBS := $(filter-out libss-render.a -lpsp% -lstdc++ -lm,$(shell sed -n 's/^LIBS = //p' $(SSMK) 2>/dev/null))
SSWRAP := $(filter -Wl$(COMMA)--wrap=%,$(shell sed -n 's/^LDFLAGS = //p' $(SSMK) 2>/dev/null))
DEFS   := -O2 -DSDK_PORT -DSDK_X86 -DSDK_TS -DSDK_4M -DSDK_FINALROM -DNNS_FINALROM -DSDK_VERSION_MAJOR=4 -DSDL_MAIN_HANDLED -DMELONDS_PSP -DPSP_NATIVE_RGBA_OUTPUT=1 -DSS3DS -ffunction-sections -fdata-sections
INC    := -I$(W)/native-probe/backing/include -I$(W)/native-graphics/alias-proof/include -I$(W)/native-probe/generated -I$(W)/native-graphics/libntr/include -I$(W)/../melon -I$(W)/native-render-opt -I$(W)/soulsilver-native-core/include -I$(W)/soulsilver-native-core/game-include
PLAT   := ../platform
POBJS  := build/main3ds.o build/logring.o build/psp_ctru.o build/sdl_ctru.o build/stubs3ds.o build/render3ds.o build/g3soft.o build/gpuscale.o build/blit_shbin.o build/g3gpu.o build/g3_shbin.o build/gpucomp.o build/romcache3ds.o build/p3ds_gamefont.o build/psp_ss_ctru.o
# melonDS 2D core + SoulSilver's 3D handler/frontend (the parts of libss-render.a that are not PSP GE code).
RENDER := $(W)/native-render-opt/native_gpu.o $(W)/native-render-opt/GPU2D_Soft.o $(F)/g3_handler.o $(F)/frontend.o
# Platinum 3DS link wraps that are about the platform, not the game (game-specific ones get re-checked on SoulSilver).
P3WRAP := -Wl,--wrap=abort,--wrap=OS_WaitVBlankIntr,--wrap=OS_WaitIrq,--wrap=MI_WaitDma,--wrap=printf,--wrap=puts,--wrap=PSPNativeFatal,--wrap=CTRDG_Init
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
build/sdl_ctru.o build/psp_ctru.o build/psp_ss_ctru.o: INC :=
# libctru/citro3d ABI uses short enums; game-facing code uses int enums (pspoke/DS data layout).
build/sdl_ctru.o build/psp_ctru.o build/psp_ss_ctru.o build/gpuscale.o: DEFS += -fshort-enums
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
build/p3ds.specs:
	@mkdir -p build; cp $(DKP)/devkitARM/arm-none-eabi/lib/3dsx.specs $@
soulsilver3ds.elf: check $(POBJS) build/p3ds.specs
	cd $(P) && $(DKA)gcc -specs=$(CURDIR)/build/p3ds.specs -B$(DKP)/devkitARM/arm-none-eabi/lib/ $(ARCH) -Wl,--gc-sections $(SSWRAP) $(P3WRAP) -Wl,--no-enum-size-warning -Wl,-Map,$(CURDIR)/build/full.map \
	  $(addprefix $(CURDIR)/,$(POBJS)) $(SSOBJS) $(RENDER) $(SSLIBS) -L$(DKP)/libctru/lib -lcitro3d -lctru -lstdc++ -lm -o $(CURDIR)/$@
soulsilver3ds.smdh:
	$(DKP)/tools/bin/smdhtool --create "SoulSilver 3DS (dev)" "native New 3DS port" "local build" $(DKP)/libctru/default_icon.png $@
soulsilver3ds.3dsx: soulsilver3ds.elf soulsilver3ds.smdh
	$(DKP)/tools/bin/3dsxtool $< $@ --smdh=soulsilver3ds.smdh
clean:
	rm -rf build soulsilver3ds.*
.PHONY: all check clean
