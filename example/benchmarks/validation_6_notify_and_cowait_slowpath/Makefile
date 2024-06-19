ifndef MICROKIT_SDK
	$(error MICROKIT SDK must be specified)
endif 

ifndef TOOLCHAIN
	$(error TOOLCHAIN must be specified)
endif 

OPENSBI ?= /home/billn/opensbi
PWD := $(shell pwd)

MICROKIT_CONFIG := benchmark
BUILD_DIR := $(PWD)/build

CC := $(TOOLCHAIN)-gcc
LD := $(TOOLCHAIN)-ld
MICROKIT_TOOL = $(MICROKIT_SDK)/bin/microkit

CC_INCLUDE_SERIAL := -Iserial_drv/include

CC_INCLUDE_MICROKIT_FLAG = -I$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/include
CFLAGS =  $(ECFLAGS) -c -O2 -mstrict-align -nostdlib -ffreestanding -Wall -Wno-array-bounds -Wno-unused-function -I. $(CC_INCLUDE_MICROKIT_FLAG) $(CC_INCLUDE_SERIAL)
LDFLAGS = -L$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/lib 
LIBS = -lmicrokit -Tmicrokit.ld 

.PHONY: directories

directories:
	$(info $(shell mkdir -p $(BUILD_DIR)))

LIBMICROKITCO_PATH := ../../../
LIBMICROKITCO_OBJ := $(BUILD_DIR)/libmicrokitco/libmicrokitco_$(TARGET).o
LIBMICROKITCO_OPT_PATH := $(PWD)

export LIBMICROKITCO_PATH LIBMICROKITCO_OPT_PATH MICROKIT_SDK BUILD_DIR MICROKIT_BOARD MICROKIT_CONFIG CPU TOOLCHAIN
$(LIBMICROKITCO_OBJ):
	make -f $(LIBMICROKITCO_PATH)/Makefile TARGET=$(TARGET)

$(BUILD_DIR)/putchar_serial.o: serial_drv/putchar_serial.c
	$(CC) $(CFLAGS) $(SERIAL_CONFIG) $^ -o $@

$(BUILD_DIR)/printf.o: serial_drv/printf.c
	$(CC) $(CFLAGS) -DPRINTF_DISABLE_SUPPORT_FLOAT $^ -o $@

$(BUILD_DIR)/start.o: start.c
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/client.o: client.c
	$(CC) $(CFLAGS) -I$(LIBMICROKITCO_PATH) $^ -o $@

$(BUILD_DIR)/server.o: server.c
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/start.elf: $(BUILD_DIR)/start.o
	$(LD) $(LDFLAGS) $(LIBS) $^ -o $@

$(BUILD_DIR)/client.elf: $(BUILD_DIR)/client.o $(BUILD_DIR)/putchar_serial.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_OBJ)
	$(LD) $(LDFLAGS) $(LIBS) $^ -o $@

$(BUILD_DIR)/server.elf: $(BUILD_DIR)/server.o $(BUILD_DIR)/putchar_serial.o $(BUILD_DIR)/printf.o
	$(LD) $(LDFLAGS) $(LIBS) $^ -o $@

.PHONY: build_odroidc4
build_odroidc4: MICROKIT_BOARD = odroidc4
build_odroidc4: CPU = cortex-a55
build_odroidc4: ECFLAGS = '-mtune=cortex-a55'
build_odroidc4: SERIAL_CONFIG = -DCONFIG_PLAT_ODROIDC4
build_odroidc4: directories $(BUILD_DIR)/server.elf $(BUILD_DIR)/client.elf $(BUILD_DIR)/start.elf
# A bit of a hack to map the correct serial device
	sed -E 's/phys_addr=\"0x[0-9a-f]{4}_[0-9a-f]{4}/phys_addr=\"0xff80_3000/' validation_5_notify_and_cowait_slowpath.system >temp.system
	mv temp.system validation_5_notify_and_cowait_slowpath.system
	$(MICROKIT_TOOL) validation_5_notify_and_cowait_slowpath.system --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(BUILD_DIR)/loader.img -r $(BUILD_DIR)/report.txt

.PHONY: build_hifive
build_hifive: MICROKIT_BOARD = hifive_unleashed
build_hifive: CPU = medany
build_hifive: ECFLAGS = -mcmodel=$(shell echo $(CPU) | tr A-Z a-z) -mabi=lp64 -march=rv64imac
build_hifive: SERIAL_CONFIG = -DCONFIG_PLAT_HIFIVE
build_hifive: directories $(BUILD_DIR)/server.elf $(BUILD_DIR)/client.elf $(BUILD_DIR)/start.elf
# A bit of a hack to map the correct serial device
	sed -E 's/phys_addr=\"0x[0-9a-f]{4}_[0-9a-f]{4}/phys_addr=\"0x1001_0000/' validation_5_notify_and_cowait_slowpath.system >temp.system
	mv temp.system validation_5_notify_and_cowait_slowpath.system
	$(MICROKIT_TOOL) validation_5_notify_and_cowait_slowpath.system --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(BUILD_DIR)/loader.img -r $(BUILD_DIR)/report.txt
	make -C $(OPENSBI) -j1 PLATFORM=generic FW_PAYLOAD_PATH=$(PWD)/build/loader.img PLATFORM_RISCV_XLEN=64 PLATFORM_RISCV_ISA=rv64imac PLATFORM_RISCV_ABI=lp64 O=$(PWD)/build CROSS_COMPILE=$(TOOLCHAIN)-
