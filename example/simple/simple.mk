#
# Copyright 2025, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#

MICROKIT_CONFIG ?= debug
BUILD_DIR ?= build
PWD := $(shell pwd)

CC = clang
LD = ld.lld
OBJCOPY = llvm-objcopy

MICROKIT_TOOL = $(MICROKIT_SDK)/bin/microkit
BOARD_DIR := $(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)

ARCH := ${shell grep 'CONFIG_SEL4_ARCH  ' $(BOARD_DIR)/include/kernel/gen_config.h | cut -d' ' -f4}

LIBMICROKITCO_PATH := $(abspath ../../..)
EXAMPLE_DIR := $(abspath ../)

CC_INCLUDE_MICROKIT_FLAG = -I$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/include
CFLAGS = -target $(TARGET)
CFLAGS += -c -g
CFLAGS += -nostdlib -ffreestanding 
CFLAGS += -Wall
CFLAGS += $(CC_INCLUDE_MICROKIT_FLAG) -I$(EXAMPLE_DIR)/include

LDFLAGS := -L$(BOARD_DIR)/lib -L$(SDDF)/lib
LIBS := --start-group -lmicrokit -Tmicrokit.ld --end-group

ifeq ($(ARCH),aarch64)
	CFLAGS += -mcpu=cortex-a53 -mstrict-align -target aarch64-none-elf
else ifeq ($(ARCH),riscv64)
	CFLAGS += -march=rv64imafdc -target riscv64-none-elf
else ifeq ($(ARCH),x86_64)
	CFLAGS += -march=x86-64 -mtune=generic -target x86_64-unknown-elf
endif

IMAGE = loader.img

clean:
	rm -rfd build

# Build client system
printf.o: $(EXAMPLE_DIR)/printf.c
	$(CC) $(CFLAGS) $^ -o $@
client.o: $(EXAMPLE_DIR)/client.c
	$(CC) $(CFLAGS) -I$(LIBMICROKITCO_PATH) $^ -o $@

# Build libmicrokitco
LIBMICROKITCO_OPT_PATH := $(PWD)/include
include $(LIBMICROKITCO_PATH)/libmicrokitco.mk

# Link everything together
client.elf: client.o printf.o libmicrokitco.a
	${LD} -o $@ ${LDFLAGS} $^ ${LIBS}

# Build bootable image
$(IMAGE): client.elf $(EXAMPLE_DIR)/simple.system
	$(MICROKIT_TOOL) $(EXAMPLE_DIR)/simple.system --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $@ -r $(BUILD_DIR)/report.txt

# Run them on QEMU
run_qemu_aarch64: $(IMAGE)
	qemu-system-aarch64 -machine virt,virtualization=on        \
		-cpu cortex-a53                                        \
		-serial mon:stdio                                      \
		-device loader,file=$(IMAGE),addr=0x70000000,cpu-num=0 \
		-m size=2G                                             \
		-nographic

run_qemu_x86_64: $(IMAGE)
	$(OBJCOPY) -O elf32-i386 $(BOARD_DIR)/elf/sel4.elf $(BUILD_DIR)/sel4.elf
	qemu-system-x86_64                                                          \
		-cpu qemu64,+fsgsbase,+pdpe1gb,+pcid,+invpcid,+xsave,+xsaves,+xsaveopt  \
		-m "3G"                                                                 \
		-display none                                                           \
		-serial mon:stdio                                                       \
		-kernel $(BUILD_DIR)/sel4.elf \
		-initrd $(IMAGE)

run_qemu_riscv64: $(IMAGE)
	qemu-system-riscv64 -machine virt \
		-serial mon:stdio             \
		-kernel $(IMAGE)              \
		-m size=3G                    \
		-nographic
