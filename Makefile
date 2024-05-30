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
$(error LIBMICROKITCO_TARGET is not set)
endif

ifndef LIBMICROKITCO_PATH
$(error LIBMICROKITCO_PATH is not set)
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

ifdef LLVM
CO_CC := clang
CO_LD := ld.lld
CO_CFLAGS = -target $(TARGET)
CO_LDFLAGS = -Wno-unused-command-line-argument
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
LIBMICROKITCO_BARE_OBJ := $(LIBMICROKITCO_BUILD_DIR)/libmicrokitco_bare_$(LIBMICROKITCO_MAX_COTHREADS)ct_$(TARGET).o
LIBMICROKITCO_FINAL_OBJ := $(LIBMICROKITCO_BUILD_DIR)/libmicrokitco_$(LIBMICROKITCO_MAX_COTHREADS)ct_$(TARGET).o

CO_CFLAGS += -c -O2 -nostdlib -ffreestanding -Wall -Werror -Wno-unused-function

ifeq (aarch64,$(findstring aarch64,$(TARGET)))
CO_CFLAGS += -mtune=$(shell echo $(CPU) | tr A-Z a-z) -mstrict-align
else ifeq (x86_64,$(findstring x86_64,$(TARGET)))
CO_CFLAGS += -mtune=$(shell echo $(CPU) | tr A-Z a-z) -Wno-parentheses
else ifeq (riscv64,$(findstring riscv64,$(TARGET)))
CO_CFLAGS += -mcmodel=$(shell echo $(CPU) | tr A-Z a-z) -mstrict-align -march=rv64imac -mabi=lp64
else
$(error Unsupported target: TARGET="$(TARGET)" not subset of { "aarch64-none-elf", "x86_64-none-elf", "riscv64-none-elf" })
endif

CO_CC_INCLUDE_MICROKIT_FLAG := -I$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/include

all: libmicrokitco_directory $(LIBMICROKITCO_FINAL_OBJ)

.PHONY: libmicrokitco_directory
libmicrokitco_directory: 
	$(info $(shell mkdir -p $(BUILD_DIR)/libmicrokitco))

$(LIBCO_OBJ): $(LIBMICROKITCO_PATH)/libco/libco.c
	$(CO_CC) $(CO_CFLAGS) -Wno-unused-value $^ -o $@

$(LIBMICROKITCO_BARE_OBJ): $(LIBMICROKITCO_PATH)/libmicrokitco.c
	$(CO_CC) $(CO_CFLAGS) $(CO_CC_INCLUDE_MICROKIT_FLAG) $(UNSAFE) $(MAX_COTHREADS) $^ -o $@

$(LIBMICROKITCO_FINAL_OBJ): $(LIBCO_OBJ) $(LIBMICROKITCO_BARE_OBJ)
	$(CO_LD) $(CO_LDFLAGS) -r $^ -o $@
