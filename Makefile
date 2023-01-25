CFLAGS = -Isaturn -Imath
OPT = -O1

LIBGCC = $(shell $(CC) -print-file-name=libgcc.a)

all: raytracing.iso

LIB = ./saturn
include $(LIB)/common.mk

sh/lib1funcs.o: CFLAGS += -DL_ashiftrt

raytracing.elf: main-saturn.o raytracing.o sh/lib1funcs.o

# clean
clean: clean-sh
clean-sh:
	rm -f sh/*.o
