# Specify MICROKIT_SDK if your setup is different:
ifndef MICROKIT_SDK
	MICROKIT_SDK := /Users/dreamliner787-9/TS/sdk_x86_64
endif

TOOLCHAIN := x86_64-elf
MICROKIT_BOARD := x86_64_virt
MICROKIT_CONFIG := debug
BUILD_DIR := build
CPU := Nehalem

LIBMICROKITCO_PATH := ../../
LIBMICROKITCO_OBJ := $(BUILD_DIR)/libmicrokitco/libmicrokitco_3ct_x86_64.o
LIBMICROKITCO_MAX_COTHREADS=3
LIBMICROKITCO_TARGET=x86_64

CC := $(TOOLCHAIN)-gcc
LD := $(TOOLCHAIN)-ld
MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit

CC_INCLUDE_MICROKIT_FLAG := -I$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/include
CFLAGS := -c -mtune=$(shell echo $(CPU) | tr A-Z a-z) -nostdlib -ffreestanding -g -Wall -Wno-array-bounds -Wno-unused-variable -Wno-unused-function $(CC_INCLUDE_MICROKIT_FLAG) -Iinclude
LDFLAGS := -L$(BOARD_DIR)/lib
LIBS := -lmicrokit -Tmicrokit.ld

all: clean directories run

clean:
	rm -rfd build

directories:
	$(info $(shell mkdir -p $(BUILD_DIR)))

run: $(BUILD_DIR)/kernel.elf $(BUILD_DIR)/capdl-initialiser-with-spec.elf
	x86_64-elf-objcopy -O elf32-i386 build/sel4.elf build/kernel.elf
	qemu-system-x86_64 \
        -cpu "$(CPU)",-vme,+pdpe1gb,-xsave,-xsaveopt,-xsavec,+fsgsbase,-invpcid,+syscall,+lm,enforce \
        -m "3G"                                                                                      \
        -display none                                                                                \
        -serial mon:stdio                                                                            \
    	-kernel build/kernel.elf                                                                     \
    	-initrd build/capdl-initialiser-with-spec.elf

export LIBMICROKITCO_PATH MICROKIT_SDK TOOLCHAIN BUILD_DIR MICROKIT_BOARD MICROKIT_CONFIG CPU LIBMICROKITCO_MAX_COTHREADS LIBMICROKITCO_TARGET
$(LIBMICROKITCO_OBJ):
	make -f $(LIBMICROKITCO_PATH)/Makefile

$(BUILD_DIR)/printf.o: printf.c
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/client.o: client.c
	$(CC) $(CFLAGS) -I$(LIBMICROKITCO_PATH) $^ -o $@

$(BUILD_DIR)/client.elf: $(BUILD_DIR)/client.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_OBJ)
	$(LD) -L$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/lib $^ $(LIBS) -o $@

$(BUILD_DIR)/kernel.elf $(BUILD_DIR)/capdl-initialiser-with-spec.elf: $(BUILD_DIR)/client.elf
	$(MICROKIT_TOOL) simple.system --capdl --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(BUILD_DIR)/loader.img -r $(BUILD_DIR)/report.txt