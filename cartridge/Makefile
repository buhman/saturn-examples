LIB = ../saturn

### architecture-specific flags ###

AARCH = --isa=sh2 --big

CARCH = -m2 -mb

### general flags ###

OPT ?= -O2

DEBUG = -g -gdwarf-4

AFLAGS += --fatal-warnings

CFLAGS += -falign-functions=4 -ffunction-sections -fdata-sections -fshort-enums -ffreestanding -nostdlib
CFLAGS += -Wall -Werror -Wfatal-errors
CFLAGS += -Wno-array-bounds
CFLAGS += -I$(LIB)

CXXFLAGS += -fno-exceptions -fno-non-call-exceptions -fno-rtti -fno-threadsafe-statics

LDFLAGS += --gc-sections --print-gc-sections --no-warn-rwx-segment --print-memory-usage --entry=_start --orphan-handling=error
LDFLAGS += -L$(LIB)

DEPFLAGS = -MMD -MP

### target ###

TARGET = sh2-none-elf-
CC = $(TARGET)gcc
CXX = $(TARGET)g++
AS = $(TARGET)as
LD = $(TARGET)ld
OBJCOPY = $(TARGET)objcopy
OBJDUMP = $(TARGET)objdump

### rules ###

%.o: %.s
	$(AS) $(AARCH) $(AFLAGS) $(DEBUG) $< -o $@

%.o: %.c
	$(CC) $(CARCH) $(CFLAGS) $(OPT) $(DEBUG) $(DEPFLAGS) -MF ${<}.d -c $< -o $@

%.o: %.cpp
	$(CXX) $(CARCH) $(CFLAGS) $(CXXFLAGS) $(OPT) $(DEBUG) $(DEPFLAGS) -MF ${<}.d -c $< -o $@

%.elf:
	$(LD) $(LDFLAGS) -T $(LDSCRIPT) $^ -o $@

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
	du -b $@

%.ss: %.bin
	cp $< $@

%.pcm.o: %.pcm
	$(OBJCOPY) \
		-I binary -O elf32-sh -B sh2 \
		--rename-section .data=.rom.$(basename $@) \
		$< $@

%.adpcm.o: %.adpcm
	$(OBJCOPY) \
		-I binary -O elf32-sh -B sh2 \
		--rename-section .data=.rom.$(basename $@) \
		$< $@

### object files ###

SYS_IP_OBJ += $(LIB)/ip/sys_id.o
SYS_IP_OBJ += $(LIB)/ip/segasmp/sys_sec.o
SYS_IP_OBJ += $(LIB)/ip/sys_area.o
SYS_IP_OBJ += $(LIB)/ip/sys_areb.o
SYS_IP_OBJ += $(LIB)/ip/sys_aree.o
SYS_IP_OBJ += $(LIB)/ip/sys_arej.o
SYS_IP_OBJ += $(LIB)/ip/sys_arek.o
SYS_IP_OBJ += $(LIB)/ip/sys_arel.o
SYS_IP_OBJ += $(LIB)/ip/sys_aret.o
SYS_IP_OBJ += $(LIB)/ip/sys_areu.o
SYS_IP_OBJ += $(LIB)/ip/sys_init.o

COMMON_OBJ = start.o runtime.o

CARTRIDGE_OBJ = main.o wc3.pcm.o
cartridge.elf: LDSCRIPT = $(LIB)/cartridge.lds
cartridge.elf: $(SYS_IP_OBJ) $(COMMON_OBJ) $(CARTRIDGE_OBJ)

ADPCM_OBJ = adpcm.o ecclesia.adpcm.o
adpcm.elf: LDSCRIPT = $(LIB)/cartridge.lds
adpcm.elf: $(SYS_IP_OBJ) $(COMMON_OBJ) $(ADPCM_OBJ)

.SUFFIXES:
.INTERMEDIATE:
.SECONDARY:
.PHONY: all clean

%: RCS/%,v
%: RCS/%
%: %,v
%: s.%
%: SCCS/s.%
