# Copyright 2024, UNSW
# SPDX-License-Identifier: BSD-2-Clause

ifndef MICROKIT_SDK
$(error MICROKIT_SDK is not set)
endif

ifndef BUILD_DIR 
$(error TOOLCHAIN is not set)
endif

ifndef MICROKIT_BOARD 
$(error MICROKIT_BOARD is not set)
endif

ifndef MICROKIT_CONFIG 
$(error MICROKIT_CONFIG is not set)
endif

ifndef CPU 
$(error CPU is not set)
endif

ifndef TARGET
$(error TARGET is not set)
endif

# Absolute path to root directory of libmicrokitco
ifndef LIBMICROKITCO_PATH
$(error LIBMICROKITCO_PATH is not set)
endif

# Absolute path to the directory containing libmicrokitco_opts.h
ifndef LIBMICROKITCO_OPT_PATH
$(error LIBMICROKITCO_OPT_PATH is not set)
else
CO_CC_INCLUDE_OPT_FLAG = -I$(LIBMICROKITCO_OPT_PATH)
endif

ifndef LIBCO_PATH
LIBCO_PATH := $(LIBMICROKITCO_PATH)/libco
endif

ifdef LLVM
CO_CC := clang
CO_LD := ld.lld
CO_CFLAGS = -target $(TARGET) -Wno-unused-command-line-argument
else
ifndef TOOLCHAIN
$(error your TOOLCHAIN triple must be specified for non-LLVM toolchain setup. E.g. TOOLCHAIN = aarch64-none-elf)
else

CO_CC := $(TOOLCHAIN)-gcc
CO_LD := $(TOOLCHAIN)-ld
CO_LDFLAGS = 

endif
endif

LIBMICROKITCO_BUILD_DIR := $(BUILD_DIR)/libmicrokitco

LIBCO_OBJ := $(LIBMICROKITCO_BUILD_DIR)/libco_$(TARGET).o
LIBMICROKITCO_BARE_OBJ := $(LIBMICROKITCO_BUILD_DIR)/libmicrokitco_bare_$(TARGET).o
LIBMICROKITCO_FINAL_OBJ := $(LIBMICROKITCO_BUILD_DIR)/libmicrokitco_$(TARGET).o

CO_CFLAGS += -c -O2 -nostdlib -ffreestanding -Wall -Werror -Wno-unused-function -Wno-unused-variable

ifeq (aarch64,$(findstring aarch64,$(TARGET)))
CO_CFLAGS += -mtune=$(shell echo $(CPU) | tr A-Z a-z) -mstrict-align
else ifeq (x86_64,$(findstring x86_64,$(TARGET)))
CO_CFLAGS += -mtune=$(shell echo $(CPU) | tr A-Z a-z) -Wno-parentheses
else ifeq (riscv64,$(findstring riscv64,$(TARGET)))
# add the "d" extension after the "a" in -march if you want 64-bits hard float support
CO_CFLAGS += -mcmodel=$(shell echo $(CPU) | tr A-Z a-z) -mstrict-align -march=rv64imac -mabi=lp64
else
$(error Unsupported target: TARGET="$(TARGET)")
endif

CO_CC_INCLUDE_MICROKIT_FLAG := -I$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/include
CO_CC_INCLUDE_LIBCO_FLAG := -I$(LIBCO_PATH)

all: libmicrokitco_directory $(LIBMICROKITCO_FINAL_OBJ)

.PHONY: libmicrokitco_directory
libmicrokitco_directory: 
	$(info $(shell mkdir -p $(BUILD_DIR)/libmicrokitco))

$(LIBCO_OBJ): $(LIBCO_PATH)/libco.c
	$(CO_CC) $(CO_CFLAGS) -Wno-unused-value $^ -o $@

$(LIBMICROKITCO_BARE_OBJ): $(LIBMICROKITCO_PATH)/libmicrokitco.c $(LIBMICROKITCO_OPT_PATH)/libmicrokitco_opts.h
	$(CO_CC) $(CO_CFLAGS) $(CO_CC_INCLUDE_LIBCO_FLAG) $(CO_CC_INCLUDE_MICROKIT_FLAG) $(CO_CC_INCLUDE_OPT_FLAG) $< -o $@

$(LIBMICROKITCO_FINAL_OBJ): $(LIBCO_OBJ) $(LIBMICROKITCO_BARE_OBJ)
	$(CO_LD) $(CO_LDFLAGS) -r $^ -o $@
	rm $(LIBCO_OBJ) $(LIBMICROKITCO_BARE_OBJ)
