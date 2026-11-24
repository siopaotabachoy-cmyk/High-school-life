.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/3ds_rules

TARGET          := mossyhollow
APP_TITLE       := Mossy Hollow
APP_DESCRIPTION := A cozy top-down exploration game
APP_AUTHOR      := You

CTRULIB  := $(DEVKITPRO)/libctru
PORTLIBS ?= $(DEVKITPRO)/portlibs/3ds
ICON     := $(CTRULIB)/default_icon.png

CC := $(DEVKITARM)/bin/arm-none-eabi-gcc

ARCH    := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS  := -g -Wall -O2 -mword-relocations -ffunction-sections -fdata-sections $(ARCH) -D__3DS__ \
           -I$(CTRULIB)/include -I$(PORTLIBS)/include
LDFLAGS := -specs=3dsx.specs -g $(ARCH) -Wl,--gc-sections -L$(CTRULIB)/lib -L$(PORTLIBS)/lib
LIBS    := -lcitro2d -lcitro3d -lctru -lm

SRC := $(wildcard source/*.c)
OBJ := $(patsubst source/%.c,build/%.o,$(SRC))

all: $(TARGET).3dsx

$(TARGET).3dsx: $(TARGET).elf $(TARGET).smdh
	3dsxtool $(TARGET).elf $@ --smdh=$(TARGET).smdh

$(TARGET).smdh:
	smdhtool --create "$(APP_TITLE)" "$(APP_DESCRIPTION)" "$(APP_AUTHOR)" $(ICON) $@

$(TARGET).elf: $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) $(LIBS) -o $@

build/%.o: source/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build $(TARGET).elf $(TARGET).3dsx $(TARGET).smdh

.PHONY: all clean
