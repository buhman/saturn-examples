CFLAGS = -I../saturn -fstack-usage
OPT = -O3
LIBGCC = $(shell $(CC) -print-file-name=libgcc.a)
LIB = ../saturn

all:

include $(LIB)/m68k/common.mk

# 200 bytes
%-44100-s16be-1ch-100sample.pcm: Makefile
	sox \
		-r 44100 -e signed-integer -b 16 -c 1 -n -B \
		$@.raw \
		synth 100s $* 440 vol -10dB
	mv $@.raw $@

%.pcm.o: %.pcm
	$(BUILD_BINARY_O)

%.mid.o: %.mid
	$(BUILD_BINARY_O)

slot.elf: slot.o sine-44100-s16be-1ch-1sec.pcm.o

interrupt.elf: interrupt.o jojo-11025-s16be-1ch.pcm.o

midi.elf: midi.o sine-44100-s16be-1ch-100sample.pcm.o midi_test-c-major-scale.mid.o f2.mid.o ../midi/parse.o $(LIBGCC)
