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

ifndef BOARD 
$(error BOARD is not set)
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

CO_CC := $(TOOLCHAIN)-gcc
CO_LD := $(TOOLCHAIN)-ld

CO_CFLAGS := -c -mcpu=$(CPU) -O2 -mstrict-align -nostdlib -ffreestanding -g -Wall -Wno-array-bounds -Wno-unused-variable -Wno-unused-function
CO_CC_INCLUDE_MICROKIT_FLAG := -I$(MICROKIT_SDK)/board/$(BOARD)/$(MICROKIT_CONFIG)/include

all: directories $(BUILD_DIR)/libmicrokitco.o

$(BUILD_DIR)/libco.o: $(LIBMICROKITCO_PATH)/libco/libco.c
	$(CO_CC) $(CO_CFLAGS) -w $< -o $@

$(BUILD_DIR)/libmicrokitco_bare.o: $(LIBMICROKITCO_PATH)/libmicrokitco.c
	$(CO_CC) $(CO_CFLAGS) -Werror $(CO_CC_INCLUDE_MICROKIT_FLAG) $(UNSAFE) $< -o $@

$(BUILD_DIR)/libmicrokitco.o: $(BUILD_DIR)/libco.o $(BUILD_DIR)/libmicrokitco_bare.o
	$(CO_LD) -r $(BUILD_DIR)/libco.o $(BUILD_DIR)/libmicrokitco_bare.o -o $(BUILD_DIR)/libmicrokitco.o
