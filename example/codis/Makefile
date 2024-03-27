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

PWD := $(shell pwd)

MICROKIT_BOARD := qemu_arm_virt
MICROKIT_CONFIG := debug
BUILD_DIR := $(PWD)/build
CPU := cortex-a53

LIBMICROKITCO_PATH := $(PWD)/../..
LIBMICROKITCO_OBJ := libmicrokitco.o
include $(LIBMICROKITCO_PATH)/Makefile

CC := $(TOOLCHAIN)-gcc
LD := $(TOOLCHAIN)-ld
MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit

CC_INCLUDE_MICROKIT_FLAG := -I$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/include
CFLAGS := -c -mcpu=$(CPU) -mstrict-align -nostdlib -ffreestanding -g -Wall -Wno-array-bounds -Wno-unused-function -Werror -Iinclude $(CC_INCLUDE_MICROKIT_FLAG)
LDFLAGS := -L$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/lib
LIBS := -lmicrokit -Tmicrokit.ld -L. -lc

all: directories $(BUILD_DIR)/loader.img

directories:
	$(info $(shell mkdir -p $(BUILD_DIR)))

$(BUILD_DIR)/printf.o: printf.c
	$(CC) $(CFLAGS) printf.c -o $(BUILD_DIR)/printf.o

$(BUILD_DIR)/client1.o: client1.c
	$(CC) $(CFLAGS) -I$(LIBMICROKITCO_PATH) client1.c -o $(BUILD_DIR)/client1.o

$(BUILD_DIR)/client2.o: client2.c
	$(CC) $(CFLAGS) -I$(LIBMICROKITCO_PATH) client2.c -o $(BUILD_DIR)/client2.o

$(BUILD_DIR)/client3.o: client3.c
	$(CC) $(CFLAGS) -I$(LIBMICROKITCO_PATH) client3.c -o $(BUILD_DIR)/client3.o


$(BUILD_DIR)/server.o: server.c
	$(CC) $(CFLAGS) -I$(LIBMICROKITCO_PATH) server.c -o $(BUILD_DIR)/server.o

$(BUILD_DIR)/client1.elf: $(BUILD_DIR)/client1.o $(BUILD_DIR)/printf.o
	$(LD) $(LDFLAGS) $(BUILD_DIR)/libmicrokitco/libmicrokitco.o $(BUILD_DIR)/client1.o $(BUILD_DIR)/printf.o $(LIBS) -o $(BUILD_DIR)/client1.elf

$(BUILD_DIR)/client2.elf: $(BUILD_DIR)/client2.o $(BUILD_DIR)/printf.o
	$(LD) $(LDFLAGS) $(BUILD_DIR)/libmicrokitco/libmicrokitco.o $(BUILD_DIR)/client2.o $(BUILD_DIR)/printf.o $(LIBS) -o $(BUILD_DIR)/client2.elf

$(BUILD_DIR)/client3.elf: $(BUILD_DIR)/client3.o $(BUILD_DIR)/printf.o
	$(LD) $(LDFLAGS) $(BUILD_DIR)/libmicrokitco/libmicrokitco.o $(BUILD_DIR)/client3.o $(BUILD_DIR)/printf.o $(LIBS) -o $(BUILD_DIR)/client3.elf


$(BUILD_DIR)/server.elf: $(BUILD_DIR)/server.o $(BUILD_DIR)/printf.o $(BUILD_DIR)/libmicrokitco/libmicrokitco.o
	$(LD) $(LDFLAGS) $(BUILD_DIR)/libmicrokitco/libmicrokitco.o $(BUILD_DIR)/server.o $(BUILD_DIR)/printf.o $(LIBS) -o $(BUILD_DIR)/server.elf

$(BUILD_DIR)/loader.img: $(BUILD_DIR)/server.elf $(BUILD_DIR)/client1.elf $(BUILD_DIR)/client2.elf $(BUILD_DIR)/client3.elf
	$(MICROKIT_TOOL) codis.system --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(BUILD_DIR)/loader.img -r $(BUILD_DIR)/report.txt
