DAY1_OBJ = \
	saturn/start.o \
	scu-dsp/advent/2020/day1/loader.o \
	cdc/serial.o \
	scu-dsp/advent/2020/day1/runner.dsp.o \
	scu-dsp/advent/2020/day1/input.bin.o \
	scu-dsp/vdp2_text.o \
	font/hp_100lx_4bit_flip.data.o

scu-dsp/advent/2020/day1/main.elf: $(DAY1_OBJ)
