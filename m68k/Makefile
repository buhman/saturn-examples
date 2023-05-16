CFLAGS = -I../saturn
OPT = -Og
LIB = ../saturn

all: 

include $(LIB)/m68k/common.mk

%.pcm.o: %.pcm
	$(BUILD_BINARY_O)

slot.elf: $(LIB)/m68k/vectors.o slot.o sine-44100-s16be-1ch.pcm.o
