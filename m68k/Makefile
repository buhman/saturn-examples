CFLAGS = -I../saturn
OPT = -Og
LIB = ../saturn

all:

include $(LIB)/m68k/common.mk

%.pcm.o: %.pcm
	$(BUILD_BINARY_O)

slot.elf: slot.o sine-44100-s16be-1ch-1sec.pcm.o

interrupt.elf: interrupt.o jojo-11025-s16be-1ch.pcm.o
