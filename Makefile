CFLAGS = -Isaturn
OPT = -Og
LIBGCC = $(shell $(CC) -print-file-name=libgcc.a)
LIB = ./saturn

all: 

include $(LIB)/common.mk

%.data.o: %.data
	$(BUILD_BINARY_O)

%.bin.o: %.bin
	$(BUILD_BINARY_O)

%.pcm.o: %.pcm
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

vdp1/kana.elf: vdp1/kana.o res/ipapgothic.font.bin.o sh/lib1funcs.o

res/mai.data: res/mai00.data res/mai01.data res/mai02.data res/mai03.data res/mai04.data res/mai05.data res/mai06.data res/mai07.data res/mai08.data res/mai09.data res/mai10.data res/mai11.data res/mai12.data res/mai13.data res/mai14.data res/mai15.data
	cat $(sort $^) > $@

vdp1/normal_sprite_animated.elf: vdp1/normal_sprite_animated.o res/mai.data.o res/mai.data.pal.o

smpc/input_intback.elf: smpc/input_intback.o sh/lib1funcs.o

res/dejavusansmono.font.bin: tools/ttf-convert
	./tools/ttf-convert 20 7f 22 $(shell fc-match -f '%{file}' 'DejaVu Sans Mono') $@

res/ipapgothic.font.bin: tools/ttf-convert
	./tools/ttf-convert 3000 30ff 28 $(shell fc-match -f '%{file}' 'IPAPGothic') $@

common/keyboard.hpp: common/keyboard.py
	python common/keyboard.py header > $@

common/keyboard.cpp: common/keyboard.py common/keyboard.hpp
	python common/keyboard.py definition > $@

smpc/input_keyboard.o: common/keyboard.hpp

smpc/input_keyboard.elf: smpc/input_keyboard.o sh/lib1funcs.o res/dejavusansmono.font.bin.o common/keyboard.o common/draw_font.o common/palette.o

wordle/main_saturn.o: common/keyboard.hpp

wordle/word_list.hpp: wordle/word_list.csv wordle/word_list.py
	python wordle/word_list.py > $@

wordle/wordle.o: wordle/word_list.hpp

wordle/wordle.elf: wordle/main_saturn.o wordle/wordle.o wordle/draw.o sh/lib1funcs.o res/dejavusansmono.font.bin.o common/keyboard.o common/draw_font.o common/palette.o

scsp/sine-44100-s16be-1ch.pcm:
	sox \
		-r 44100 -e signed-integer -b 16 -c 1 -n -B \
		$@.raw \
		synth 1 sin 440 vol -10dB
	mv $@.raw $@

scsp/slot.elf: scsp/slot.o scsp/sine-44100-s16be-1ch.pcm.o

scsp/sound_cpu.elf: scsp/sound_cpu.o m68k/slot.bin.o

# clean
clean: clean-sh
clean-sh:
	find -P \
		-not -path './saturn/*' \
		-not -path './tools/*' \
		-regextype posix-egrep \
		-regex '.*\.(iso|o|bin|elf|cue)$$' \
		-exec rm {} \;
	rm -f \
		common/keyboard.cpp \
		common/keyboard.hpp \
		wordle/word_list.hpp
