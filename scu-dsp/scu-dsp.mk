scu-dsp/add.elf: scu-dsp/add.o saturn/start.o cdc/serial.o scu-dsp/input.bin.o

scu-dsp/div10.elf: scu-dsp/div10.o saturn/start.o cdc/serial.o scu-dsp/div10.dsp.o

scu-dsp/div10_vdp2.elf: scu-dsp/div10_vdp2.o saturn/start.o cdc/serial.o scu-dsp/div10_vdp2.dsp.o font/hp_100lx_4bit_flip.data.o

%.dsp: %.asm
	~/scu-dsp-asm/scu-dsp-asm $< $@

%.asm: %.asm.m4
	m4 -I $(MAKEFILE_PATH)/scu-dsp -I $(dir $<) $< > $@.tmp
	mv $@.tmp $@
