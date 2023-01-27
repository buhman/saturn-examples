CFLAGS = -Isaturn
OPT = -Os
LIBGCC = $(shell $(CC) -print-file-name=libgcc.a)

all: raytracing.iso vdp2.iso

LIB = ./saturn
include $(LIB)/common.mk

sh/lib1funcs.o: CFLAGS += -DL_ashiftrt -DL_movmem

raytracing/raytracing.elf: CFLAGS += -Imath -DUSE_SH2_DVSR
raytracing/raytracing.elf: raytracing/main-saturn.o raytracing/raytracing.o sh/lib1funcs.o

# clean
clean: clean-sh
clean-sh:
	find -P \
		-not -path './saturn/*' \
		-regextype posix-egrep \
		-regex '.*\.(iso|o|bin|elf|cue)$$' \
		-exec rm {} \;
