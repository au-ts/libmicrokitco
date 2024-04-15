# Specify MICROKIT_SDK if your setup is different:
ifndef MICROKIT_SDK
	MICROKIT_SDK := ../../../sdk
endif 

TOOLCHAIN := aarch64-none-elf

PWD := $(shell pwd)

MICROKIT_BOARD := odroidc4
MICROKIT_CONFIG := benchmark
BUILD_DIR := $(PWD)/build
CPU := cortex-a53

CC := $(TOOLCHAIN)-gcc
LD := $(TOOLCHAIN)-ld
MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit

SERIAL := $(PWD)/serial_drv
CC_INCLUDE_SERIAL := -I$(SERIAL)/include

CC_INCLUDE_MICROKIT_FLAG := -I$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/include
CFLAGS := -c -mcpu=$(CPU) -O3 -mstrict-align -nostdlib -ffreestanding -Wall -Wno-array-bounds -Wno-unused-function -Iinclude $(CC_INCLUDE_MICROKIT_FLAG) $(CC_INCLUDE_SERIAL)
LDFLAGS := -L$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/lib
LIBS := -lmicrokit -Tmicrokit.ld

LIBMICROKITCO_PATH := ../../

all: directories $(BUILD_DIR)/loader.img

directories:
	$(info $(shell mkdir -p $(BUILD_DIR)))

$(BUILD_DIR)/libco.o: $(LIBMICROKITCO_PATH)/libco/libco.c
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/putchar_serial.o: serial_drv/putchar_serial.c
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/printf.o: serial_drv/printf.c
	$(CC) $(CFLAGS) -DCONFIG_PLAT_ODROIDC4 $^ -o $@

$(BUILD_DIR)/start.o: start.c
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/client.o: client.c
	$(CC) $(CFLAGS) -I$(LIBMICROKITCO_PATH) $^ -o $@

$(BUILD_DIR)/server.o: server.c
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/start.elf: $(BUILD_DIR)/start.o
	$(LD) $(LDFLAGS) $(LIBS) $^ -o $@

$(BUILD_DIR)/client.elf: $(BUILD_DIR)/client.o $(BUILD_DIR)/putchar_serial.o $(BUILD_DIR)/printf.o $(BUILD_DIR)/libco.o
	$(LD) $(LDFLAGS) $(LIBS) $^ -o $@

$(BUILD_DIR)/server.elf: $(BUILD_DIR)/server.o $(BUILD_DIR)/putchar_serial.o $(BUILD_DIR)/printf.o
	$(LD) $(LDFLAGS) $(LIBS) $^ -o $@

$(BUILD_DIR)/loader.img: $(BUILD_DIR)/server.elf $(BUILD_DIR)/client.elf $(BUILD_DIR)/start.elf
	$(MICROKIT_TOOL) validation_notify_and_bare_libco_wait.system --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(BUILD_DIR)/loader.img -r $(BUILD_DIR)/report.txt
