CFLAGS = -Isaturn
OPT = -O0
LIBGCC = $(shell $(CC) -print-file-name=libgcc.a)

all: raytracing/raytracing.iso vdp2/nbg0.iso

LIB = ./saturn
include $(LIB)/common.mk

define BUILD_BINARY_O
	$(OBJCOPY) \
		-I binary -O elf32-sh -B sh2 \
		--rename-section .data=.data.$(basename $@) \
		$< $@
endef

%.data.o: %.data
	$(BUILD_BINARY_O)

%.bin.o: %.bin
	$(BUILD_BINARY_O)

%.data.pal.o: %.data.pal
	$(BUILD_BINARY_O)

sh/lib1funcs.o: CFLAGS += -DL_ashiftrt -DL_movmem

raytracing/raytracing.elf: CFLAGS += -Imath -DUSE_SH2_DVSR
raytracing/raytracing.elf: raytracing/main-saturn.o raytracing/raytracing.o sh/lib1funcs.o

vdp2/nbg0.elf: vdp2/nbg0.o res/butterfly.data.o res/butterfly.data.pal.o

vdp1/polygon.elf: vdp1/polygon.o
vdp1/normal_sprite.elf: vdp1/normal_sprite.o res/mai00.data.o res/mai.data.pal.o

vdp1/normal_sprite_color_bank.elf: vdp1/normal_sprite_color_bank.o res/mai00.data.o res/mai.data.pal.o

vdp1/kana.elf: vdp1/kana.o res/ipafont.bin.o sh/lib1funcs.o

res/mai.data: res/mai00.data res/mai01.data res/mai02.data res/mai03.data res/mai04.data res/mai05.data res/mai06.data res/mai07.data res/mai08.data res/mai09.data res/mai10.data res/mai11.data res/mai12.data res/mai13.data res/mai14.data res/mai15.data
	cat $(sort $^) > $@

vdp1/normal_sprite_animated.elf: vdp1/normal_sprite_animated.o res/mai.data.o res/mai.data.pal.o

# clean
clean: clean-sh
clean-sh:
	find -P \
		-not -path './saturn/*' \
		-regextype posix-egrep \
		-regex '.*\.(iso|o|bin|elf|cue)$$' \
		-exec rm {} \;
