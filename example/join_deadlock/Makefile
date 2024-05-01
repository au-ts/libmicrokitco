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

CC := $(TOOLCHAIN)-gcc
LD := $(TOOLCHAIN)-ld
MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit

LIBMICROKITCO_PATH := ../../
# Our PDs have different needs for number of cothreads so we need multiple configs of the library.
LIBMICROKITCO_3T_OBJ := $(BUILD_DIR)/libmicrokitco/libmicrokitco_3_cothreads.o
LIBMICROKITCO_4T_OBJ := $(BUILD_DIR)/libmicrokitco/libmicrokitco_4_cothreads.o

CC_INCLUDE_MICROKIT_FLAG := -I$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/include
CFLAGS := -c -mcpu=$(CPU) -mstrict-align -nostdlib -ffreestanding -g -Wall -Wno-array-bounds -Wno-unused-function -Wno-format -Werror -Iinclude $(CC_INCLUDE_MICROKIT_FLAG)
LDFLAGS := -L$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/lib
LIBS := -lmicrokit -Tmicrokit.ld

all: directories $(BUILD_DIR)/loader.img

directories:
	$(info $(shell mkdir -p $(BUILD_DIR)))

export LIBMICROKITCO_PATH MICROKIT_SDK TOOLCHAIN BUILD_DIR MICROKIT_BOARD MICROKIT_CONFIG CPU
$(LIBMICROKITCO_3T_OBJ):
	make -f $(LIBMICROKITCO_PATH)/Makefile LIBMICROKITCO_MAX_COTHREADS=3

$(LIBMICROKITCO_4T_OBJ):
	make -f $(LIBMICROKITCO_PATH)/Makefile LIBMICROKITCO_MAX_COTHREADS=4

$(BUILD_DIR)/printf.o: printf.c
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/deadlocked.o: deadlocked.c
	$(CC) $(CFLAGS) -I$(LIBMICROKITCO_PATH) $^ -o $@

$(BUILD_DIR)/deadlock_free.o: deadlock_free.c
	$(CC) $(CFLAGS) -I$(LIBMICROKITCO_PATH) $^ -o $@

$(BUILD_DIR)/many.o: many.c
	$(CC) $(CFLAGS) -I$(LIBMICROKITCO_PATH) $^ -o $@

$(BUILD_DIR)/deadlocked.elf: $(BUILD_DIR)/deadlocked.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_3T_OBJ)
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/deadlock_free.elf: $(BUILD_DIR)/deadlock_free.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_3T_OBJ)
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/many.elf: $(BUILD_DIR)/many.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_4T_OBJ)
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(BUILD_DIR)/loader.img: $(BUILD_DIR)/deadlocked.elf $(BUILD_DIR)/deadlock_free.elf $(BUILD_DIR)/many.elf
	$(MICROKIT_TOOL) join_deadlock.system --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(BUILD_DIR)/loader.img -r $(BUILD_DIR)/report.txt
