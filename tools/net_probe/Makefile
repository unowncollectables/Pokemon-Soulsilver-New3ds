# net_probe: standalone libctru app (no game code). Build: make  ->  net_probe.3dsx
ifeq ($(strip $(DEVKITARM)),)
$(error "Set DEVKITARM (devkitPro 3ds-dev), e.g. export DEVKITARM=/opt/devkitpro/devkitARM")
endif
include $(DEVKITARM)/3ds_rules

TARGET      := net_probe
APP_TITLE   := net_probe
APP_DESCRIPTION := Online server reachability probe
APP_AUTHOR  := soulsilver3ds
ARCH        := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS      := -g -Wall -O2 -mword-relocations -ffunction-sections $(ARCH) -I$(CTRULIB)/include -D__3DS__
LDFLAGS     := -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(TARGET).map
LIBS        := -L$(CTRULIB)/lib -lctru -lm

all: $(TARGET).3dsx

$(TARGET).elf: source/main.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< $(LIBS) -o $@

$(TARGET).smdh:
	smdhtool --create "$(APP_TITLE)" "$(APP_DESCRIPTION)" "$(APP_AUTHOR)" $(DEVKITPRO)/libctru/default_icon.png $@

$(TARGET).3dsx: $(TARGET).elf $(TARGET).smdh
	3dsxtool $< $@ --smdh=$(TARGET).smdh

clean:
	rm -f $(TARGET).elf $(TARGET).3dsx $(TARGET).smdh $(TARGET).map

.PHONY: all clean
