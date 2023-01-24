CFLAGS = -Isaturn -Imath
OPT = -O3

all: raytracing.iso

LIB = ./saturn
include $(LIB)/common.mk

LIBGCC = $(shell $(CC) -print-file-name=libgcc.a)
raytracing.elf: main-saturn.o raytracing.o $(LIBGCC)
