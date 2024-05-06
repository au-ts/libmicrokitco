# Specify MICROKIT_SDK if your setup is different:
ifndef MICROKIT_SDK
	MICROKIT_SDK := ../../../sdk
endif

TOOLCHAIN := aarch64-none-elf
MICROKIT_BOARD := qemu_arm_virt
MICROKIT_CONFIG := debug
BUILD_DIR := build
CPU := cortex-a53

LIBMICROKITCO_PATH := ../../
LIBMICROKITCO_OBJ := $(BUILD_DIR)/libmicrokitco/libmicrokitco_3ct_aarch64.o
LIBMICROKITCO_MAX_COTHREADS=3
LIBMICROKITCO_TARGET=aarch64

CC := $(TOOLCHAIN)-gcc
LD := $(TOOLCHAIN)-ld
MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit

CC_INCLUDE_MICROKIT_FLAG := -I$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/include
CFLAGS := -c -mcpu=$(CPU) -mstrict-align -nostdlib -ffreestanding -g -Wall -Wno-array-bounds -Wno-unused-variable -Wno-unused-function $(CC_INCLUDE_MICROKIT_FLAG) -Iinclude
LDFLAGS := -L$(BOARD_DIR)/lib
LIBS := -lmicrokit -Tmicrokit.ld

all: clean directories run

clean:
	rm -rfd build

directories:
	$(info $(shell mkdir -p $(BUILD_DIR)))

run: $(BUILD_DIR)/loader.img
	qemu-system-aarch64 -machine virt,virtualization=on \
		-cpu "$(CPU)" \
		-serial mon:stdio \
		-device loader,file=build/loader.img,addr=0x70000000,cpu-num=0 \
		-m size=2G \
		-nographic

export LIBMICROKITCO_PATH MICROKIT_SDK TOOLCHAIN BUILD_DIR MICROKIT_BOARD MICROKIT_CONFIG CPU LIBMICROKITCO_MAX_COTHREADS LIBMICROKITCO_TARGET
$(LIBMICROKITCO_OBJ):
	make -f $(LIBMICROKITCO_PATH)/Makefile

$(BUILD_DIR)/printf.o: printf.c
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/client.o: client.c
	$(CC) $(CFLAGS) -I$(LIBMICROKITCO_PATH) $^ -o $@

$(BUILD_DIR)/client.elf: $(BUILD_DIR)/client.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_OBJ)
	$(LD) -L$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/lib $^ $(LIBS) -o $@

$(BUILD_DIR)/loader.img: $(BUILD_DIR)/client.elf
	$(MICROKIT_TOOL) simple.system --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(BUILD_DIR)/loader.img -r $(BUILD_DIR)/report.txt