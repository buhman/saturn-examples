CFLAGS = -Isaturn
OPT ?= -Og
LIB = ./saturn

ALL =
ALL += raytracing/raytracing.cue
ALL += vdp2/nbg0.cue
ALL += vdp1/polygon.cue
ALL += vdp1/normal_sprite.cue
ALL += vdp1/normal_sprite_color_bank.cue
ALL += vdp1/kana.cue
ALL += vdp1/normal_sprite_animated.cue
ALL += vdp1/rgb.cue
ALL += smpc/input_intback.cue
ALL += smpc/input_keyboard.cue
ALL += wordle/wordle.cue
ALL += scsp/slot.cue
ALL += scsp/sound_cpu__slot.cue
ALL += scsp/sound_cpu__interrupt.cue
ALL += scsp/sound_cpu__midi_debug.cue
ALL += editor/main_saturn.cue

all: $(ALL)

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

vdp2/nbg0_16color.elf: vdp2/nbg0_16color.o res/kirby.data.o res/kirby.data.pal.o

vdp1/polygon.elf: vdp1/polygon.o
vdp1/cube.elf: vdp1/cube.o $(LIBGCC)
vdp1/normal_sprite.elf: vdp1/normal_sprite.o res/mai00.data.o res/mai.data.pal.o

vdp1/normal_sprite_color_bank.elf: vdp1/normal_sprite_color_bank.o res/mai00.data.o res/mai.data.pal.o

vdp1/kana.elf: vdp1/kana.o res/ipapgothic.font.bin.o sh/lib1funcs.o

res/mai.data: res/mai00.data res/mai01.data res/mai02.data res/mai03.data res/mai04.data res/mai05.data res/mai06.data res/mai07.data res/mai08.data res/mai09.data res/mai10.data res/mai11.data res/mai12.data res/mai13.data res/mai14.data res/mai15.data
	cat $(sort $^) > $@

vdp1/normal_sprite_animated.elf: vdp1/normal_sprite_animated.o res/mai.data.o res/mai.data.pal.o

vdp1/rgb.elf: vdp1/rgb.o vdp1/chikorita.o

smpc/input_intback.elf: smpc/input_intback.o sh/lib1funcs.o

tools:

tools/%: tools
	$(MAKE) -C tools $(notdir $@)

res/dejavusansmono.font.bin: tools/ttf-convert
	./tools/ttf-convert 20 7f 22 $(shell fc-match -f '%{file}' 'DejaVu Sans Mono') $@

res/ipapgothic.font.bin: tools/ttf-convert
	./tools/ttf-convert 3000 30ff 28 $(shell fc-match -f '%{file}' 'IPAPGothic') $@

res/sperrypc.font.bin: tools/ttf-convert
	./tools/ttf-convert 20 7f 8 res/Bm437_SperryPC_CGA.otb $@

common/keyboard.hpp: common/keyboard.py
	python common/keyboard.py header > $@

common/keyboard.cpp: common/keyboard.py common/keyboard.hpp
	python common/keyboard.py definition > $@

smpc/input_keyboard.cpp.d: common/keyboard.hpp

smpc/input_keyboard.elf: smpc/input_keyboard.o sh/lib1funcs.o res/dejavusansmono.font.bin.o common/keyboard.o common/draw_font.o common/palette.o

wordle/main_saturn.o: common/keyboard.hpp

wordle/word_list.hpp: wordle/word_list.csv wordle/word_list.py
	python wordle/word_list.py > $@

wordle/wordle.cpp.d: wordle/word_list.hpp

wordle/wordle.elf: wordle/main_saturn.o wordle/wordle.o wordle/draw.o sh/lib1funcs.o res/dejavusansmono.font.bin.o common/keyboard.o common/draw_font.o common/palette.o

# 88200 bytes
scsp/sine-44100-s16be-1ch-1sec.pcm:
	sox \
		-r 44100 -e signed-integer -b 16 -c 1 -n -B \
		$@.raw \
		synth 1 sin 440 vol -10dB
	mv $@.raw $@

# 200 bytes
scsp/%-44100-s16be-1ch-100sample.pcm:
	sox \
		-r 44100 -e signed-integer -b 16 -c 1 -n -B \
		$@.raw \
		synth 100s $* 440 vol -10dB
	mv $@.raw $@

m68k:

m68k/%.bin: m68k
	$(MAKE) -C m68k $(notdir $@)

scsp/slot.elf: scsp/slot.o scsp/sine-44100-s16be-1ch-1sec.pcm.o

scsp/sound_cpu__slot.elf: scsp/sound_cpu__slot.o m68k/slot.bin.o

scsp/sound_cpu__interrupt.elf: scsp/sound_cpu__interrupt.o m68k/interrupt.bin.o sh/lib1funcs.o res/sperrypc.font.bin.o common/draw_font.o common/palette.o

scsp/sound_cpu__midi_debug.elf: scsp/sound_cpu__midi_debug.o m68k/midi_debug.bin.o sh/lib1funcs.o res/nec.bitmap.bin.o

scsp/fm.elf: scsp/fm.o res/nec.bitmap.bin.o sh/lib1funcs.o saturn/start.o scsp/sine-44100-s16be-1ch-100sample.pcm.o

scsp/sound_cpu__midi.elf: scsp/sound_cpu__midi.o m68k/midi.bin.o res/nec.bitmap.bin.o sh/lib1funcs.o saturn/start.o

res/sperrypc.bitmap.bin: tools/ttf-bitmap
	./tools/ttf-bitmap 20 7f res/Bm437_SperryPC_CGA.otb $@

res/nec.bitmap.bin: tools/ttf-bitmap
	./tools/ttf-bitmap 20 7f res/Bm437_NEC_MultiSpeed.otb $@

res/nec_bold.bitmap.bin: tools/ttf-bitmap
	./tools/ttf-bitmap 20 7f res/Bm437_NEC_MultiSpeed_bold.otb $@

editor/main_saturn.o: common/keyboard.hpp editor/editor.hpp

editor/main_saturn.elf: editor/main_saturn.o res/nec.bitmap.bin.o res/nec_bold.bitmap.bin.o sh/lib1funcs.o common/keyboard.o saturn/start.o

# clean
clean: clean-sh
clean-sh:
	find -P \
		-not -path './saturn/*' \
		-not -path './tools/*' \
		-regextype posix-egrep \
		-regex '.*\.(iso|o|bin|elf|cue|gch)$$' \
		-exec rm {} \;
	rm -f \
		common/keyboard.cpp \
		common/keyboard.hpp \
		wordle/word_list.hpp
	make -C tools clean
	make -C m68k clean

PHONY: m68k tools
