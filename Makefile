CFLAGS = -Isaturn
OPT = -Os
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

%.data.pal.o: %.data.pal
	$(BUILD_BINARY_O)

sh/lib1funcs.o: CFLAGS += -DL_ashiftrt -DL_movmem

raytracing/raytracing.elf: CFLAGS += -Imath -DUSE_SH2_DVSR
raytracing/raytracing.elf: raytracing/main-saturn.o raytracing/raytracing.o sh/lib1funcs.o

vdp2/nbg0.elf: vdp2/nbg0.o res/butterfly.data.o res/butterfly.data.pal.o

# clean
clean: clean-sh
clean-sh:
	find -P \
		-not -path './saturn/*' \
		-regextype posix-egrep \
		-regex '.*\.(iso|o|bin|elf|cue)$$' \
		-exec rm {} \;
