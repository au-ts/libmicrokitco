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

ifdef LIBMICROKITCO_UNSAFE
UNSAFE := -D LIBMICROKITCO_UNSAFE
else
UNSAFE :=  
endif

LIBMICROKITCO_BUILD_DIR := $(BUILD_DIR)/libmicrokitco

CO_CC := $(TOOLCHAIN)-gcc
CO_LD := $(TOOLCHAIN)-ld

CO_CFLAGS := -c -mcpu=$(CPU) -O3 -mstrict-align -nostdlib -ffreestanding -Wall -Wno-stringop-overflow -Wno-unused-function -Wno-unused-variable -Wno-array-bounds
CO_CC_INCLUDE_MICROKIT_FLAG := -I$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/include

all: libmicrokitco_directory $(BUILD_DIR)/libmicrokitco/libmicrokitco.o

libmicrokitco_directory: 
	$(info $(shell mkdir -p $(BUILD_DIR)/libmicrokitco))	

$(LIBMICROKITCO_BUILD_DIR)/libco.o: $(LIBMICROKITCO_PATH)/libco/libco.c
	$(CO_CC) $(CO_CFLAGS) -w $< -o $@

$(LIBMICROKITCO_BUILD_DIR)/libmicrokitco_bare.o: $(LIBMICROKITCO_PATH)/libmicrokitco.c
	$(CO_CC) $(CO_CFLAGS) -Werror $(CO_CC_INCLUDE_MICROKIT_FLAG) $(UNSAFE) $< -o $@

$(LIBMICROKITCO_BUILD_DIR)/libmicrokitco.o: $(LIBMICROKITCO_BUILD_DIR)/libco.o $(LIBMICROKITCO_BUILD_DIR)/libmicrokitco_bare.o
	$(CO_LD) -r $(LIBMICROKITCO_BUILD_DIR)/libco.o $(LIBMICROKITCO_BUILD_DIR)/libmicrokitco_bare.o -o $(LIBMICROKITCO_BUILD_DIR)/libmicrokitco.o
