CFLAGS = -I../saturn
OPT = -Og
LIB = ../saturn

all: 

include $(LIB)/m68k/common.mk

%.pcm.o: %.pcm
	$(BUILD_BINARY_O)

slot.elf: $(LIB)/m68k/vectors.o $(LIB)/m68k/handlers.o slot.o sine-44100-s16be-1ch.pcm.o

interrupt.elf: $(LIB)/m68k/vectors.o $(LIB)/m68k/handlers.o interrupt.o jojo-11025-s16be-1ch.pcm.o
