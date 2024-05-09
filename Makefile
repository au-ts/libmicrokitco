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

LIBMICROKITCO_BUILD_DIR := $(BUILD_DIR)/libmicrokitco

LIBCO_OBJ := $(LIBMICROKITCO_BUILD_DIR)/libco_$(TARGET).o
LIBMICROKITCO_BARE_OBJ := $(LIBMICROKITCO_BUILD_DIR)/libmicrokitco_bare_$(LIBMICROKITCO_MAX_COTHREADS)ct_$(TARGET).o
LIBMICROKITCO_FINAL_OBJ := $(LIBMICROKITCO_BUILD_DIR)/libmicrokitco_$(LIBMICROKITCO_MAX_COTHREADS)ct_$(TARGET).o

CO_CC := clang
CO_LD := ld.lld

CO_CFLAGS = -target $(TARGET) -c -O2 -nostdlib -ffreestanding -Wall -Werror -Wno-unused-function
CO_LDFLAGS =
ifeq ($(TARGET),aarch64-none-elf)
CO_CFLAGS += -mtune=$(shell echo $(CPU) | tr A-Z a-z) -mstrict-align
else ifeq ($(TARGET),x86_64-none-elf)
CO_CFLAGS += -mtune=$(shell echo $(CPU) | tr A-Z a-z)
else ifeq ($(TARGET),riscv64-none-elf)
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
