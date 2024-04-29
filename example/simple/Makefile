# Specify MICROKIT_SDK if your setup is different:
ifndef MICROKIT_SDK
	MICROKIT_SDK := ../../../sdk
endif

# In case the default compiler triple doesn't work for you or your package manager
# only has aarch64-none-elf or something, you can specifiy the toolchain.
ifndef TOOLCHAIN
	# Get whether the common toolchain triples exist
	TOOLCHAIN_AARCH64_LINUX_GNU := $(shell command -v aarch64-linux-gnu-gcc 2> /dev/null)
	TOOLCHAIN_AARCH64_UNKNOWN_LINUX_GNU := $(shell command -v aarch64-unknown-linux-gnu-gcc 2> /dev/null)
	# Then check if they are defined and select the appropriate one
	ifdef TOOLCHAIN_AARCH64_LINUX_GNU
		TOOLCHAIN := aarch64-linux-gnu
	else ifdef TOOLCHAIN_AARCH64_UNKNOWN_LINUX_GNU
		TOOLCHAIN := aarch64-unknown-linux-gnu
	else
		$(error "Could not find an AArch64 cross-compiler")
	endif
endif

MICROKIT_BOARD := qemu_arm_virt
MICROKIT_CONFIG := debug
BUILD_DIR := build
CPU := cortex-a53

LIBMICROKITCO_PATH := ../../
LIBMICROKITCO_OBJ := libmicrokitco/libmicrokitco.o
LIBMICROKITCO_MAX_COTHREADS=3
include $(LIBMICROKITCO_PATH)/Makefile

CC := $(TOOLCHAIN)-gcc
LD := $(TOOLCHAIN)-ld
MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit

CC_INCLUDE_MICROKIT_FLAG := -I$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/include
CFLAGS := -c -mcpu=$(CPU) -mstrict-align -nostdlib -ffreestanding -g -Wall -Wno-array-bounds -Wno-unused-variable -Wno-unused-function -Werror $(CC_INCLUDE_MICROKIT_FLAG) -Iinclude
LDFLAGS := -L$(BOARD_DIR)/lib
LIBS := -lmicrokit -Tmicrokit.ld

all: directories $(BUILD_DIR)/loader.img

directories:
	$(info $(shell mkdir -p $(BUILD_DIR)))

$(BUILD_DIR)/printf.o: printf.c
	$(CC) $(CFLAGS) printf.c -o $(BUILD_DIR)/printf.o

$(BUILD_DIR)/client.o: client.c
	$(CC) $(CFLAGS) -I$(LIBMICROKITCO_PATH) $< -o $@

$(BUILD_DIR)/client.elf: $(BUILD_DIR)/client.o $(BUILD_DIR)/printf.o 
	$(LD) -L$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/lib $(BUILD_DIR)/$(LIBMICROKITCO_OBJ) $(BUILD_DIR)/client.o $(BUILD_DIR)/printf.o $(LIBS) -o $(BUILD_DIR)/client.elf

$(BUILD_DIR)/loader.img: $(BUILD_DIR)/client.elf
	$(MICROKIT_TOOL) simple.system --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(BUILD_DIR)/loader.img -r $(BUILD_DIR)/report.txt
