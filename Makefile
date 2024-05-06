ifndef LIBMICROKITCO_PATH
$(error LIBMICROKITCO_PATH is not set)
endif

ifndef MICROKIT_SDK
$(error MICROKIT_SDK is not set)
endif

ifndef TOOLCHAIN 
$(error TOOLCHAIN is not set)
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

ifndef LIBMICROKITCO_TARGET
$(error LIBMICROKITCO_TARGET is not set)
endif

ifndef LIBMICROKITCO_MAX_COTHREADS 
$(error LIBMICROKITCO_MAX_COTHREADS is not set)
else
MAX_COTHREADS := -DLIBMICROKITCO_MAX_COTHREADS=$(LIBMICROKITCO_MAX_COTHREADS)
endif

ifdef LIBMICROKITCO_UNSAFE
UNSAFE := -DLIBMICROKITCO_UNSAFE
else
UNSAFE :=  
endif

LIBMICROKITCO_BUILD_DIR := $(BUILD_DIR)/libmicrokitco

LIBCO_OBJ := $(LIBMICROKITCO_BUILD_DIR)/libco.o
LIBMICROKITCO_BARE_OBJ := $(LIBMICROKITCO_BUILD_DIR)/libmicrokitco_bare_$(LIBMICROKITCO_MAX_COTHREADS)ct_$(LIBMICROKITCO_TARGET).o
LIBMICROKITCO_FINAL_OBJ := $(LIBMICROKITCO_BUILD_DIR)/libmicrokitco_$(LIBMICROKITCO_MAX_COTHREADS)ct_$(LIBMICROKITCO_TARGET).o

CO_CC := $(TOOLCHAIN)-gcc
CO_LD := $(TOOLCHAIN)-ld

CO_CFLAGS = -c -O2 -mtune=$(shell echo $(CPU) | tr A-Z a-z) -nostdlib -ffreestanding -Wall -Werror -Wno-stringop-overflow -Wno-unused-function -Wno-unused-variable -Wno-parentheses
ifeq ($(LIBMICROKITCO_TARGET),aarch64)
CO_CFLAGS += -mstrict-align
else ifeq ($(LIBMICROKITCO_TARGET),x86_64)
CO_CFLAGS += 
else ifeq ($(LIBMICROKITCO_TARGET),riscv)
CO_CFLAGS += 
else
$(error Unsupported target: LIBMICROKITCO_TARGET="$(LIBMICROKITCO_TARGET)" not subset of { "aarch64", "x86_64", "riscv" })
endif

CO_CC_INCLUDE_MICROKIT_FLAG := -I$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/include

all: libmicrokitco_directory $(LIBMICROKITCO_FINAL_OBJ)

libmicrokitco_directory: 
	$(info $(shell mkdir -p $(BUILD_DIR)/libmicrokitco))

$(LIBCO_OBJ): $(LIBMICROKITCO_PATH)/libco/libco.c
	$(CO_CC) $(CO_CFLAGS) -Wno-unused-value $^ -o $@

$(LIBMICROKITCO_BARE_OBJ): $(LIBMICROKITCO_PATH)/libmicrokitco.c
	$(CO_CC) $(CO_CFLAGS) $(CO_CC_INCLUDE_MICROKIT_FLAG) $(UNSAFE) $(MAX_COTHREADS) $^ -o $@

$(LIBMICROKITCO_FINAL_OBJ): $(LIBCO_OBJ) $(LIBMICROKITCO_BARE_OBJ)
	$(CO_LD) -r $^ -o $@
