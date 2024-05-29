ifndef MICROKIT_SDK
$(error your MICROKIT_SDK path is not set.)
endif

ifndef TARGET
$(error your TARGET triple is not set. Required for both LLVM and gcc toolchain setups.)
endif

PWD := $(shell pwd)

MICROKIT_CONFIG := debug
BUILD_DIR := $(PWD)/build

# default to building with LLVM
ifndef TOOLCHAIN
CC = clang
LD = ld.lld
OBJCP = llvm-objcopy
CFLAGS = -target $(TARGET)
else
CC = $(TOOLCHAIN)-gcc
LD = $(TOOLCHAIN)-ld
OBJCP = $(TOOLCHAIN)-objcopy
endif

MICROKIT_TOOL ?= $(MICROKIT_SDK)/bin/microkit

LIBMICROKITCO_PATH := ../../

CC_INCLUDE_MICROKIT_FLAG = -I$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/include
CFLAGS += -c -mstrict-align -nostdlib -ffreestanding -g -Wall -Wno-unused-function -Iinclude $(CC_INCLUDE_MICROKIT_FLAG)
LDFLAGS = -L$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/lib
LIBS = -lmicrokit -Tmicrokit.ld

# Our PDs have different needs for number of cothreads so we need multiple configs of the library.
LIBMICROKITCO_1T_AARCH64_OBJ := $(BUILD_DIR)/libmicrokitco/libmicrokitco_1ct_$(TARGET).o
LIBMICROKITCO_3T_AARCH64_OBJ := $(BUILD_DIR)/libmicrokitco/libmicrokitco_3ct_$(TARGET).o
LIBMICROKITCO_1T_RISCV64_OBJ := $(BUILD_DIR)/libmicrokitco/libmicrokitco_1ct_$(TARGET).o
LIBMICROKITCO_3T_RISCV64_OBJ := $(BUILD_DIR)/libmicrokitco/libmicrokitco_3ct_$(TARGET).o

AARCH64_TARGET_RULE := $(BUILD_DIR)/loader_aarch64.img
RISCV64_TARGET_RULE := $(BUILD_DIR)/loader_riscv64.img

# Preparation steps
clean:
	rm -rfd build

directories:
	$(info $(shell mkdir -p $(BUILD_DIR)))

# Build libmicrokitco
ifndef TOOLCHAIN
LLVM = 1
export LIBMICROKITCO_PATH MICROKIT_SDK BUILD_DIR MICROKIT_BOARD MICROKIT_CONFIG CPU LIBMICROKITCO_MAX_COTHREADS LLVM

else
export LIBMICROKITCO_PATH MICROKIT_SDK BUILD_DIR MICROKIT_BOARD MICROKIT_CONFIG CPU LIBMICROKITCO_MAX_COTHREADS TOOLCHAIN

endif

$(LIBMICROKITCO_1T_AARCH64_OBJ):
	make -f $(LIBMICROKITCO_PATH)/Makefile LIBMICROKITCO_MAX_COTHREADS=1 TARGET=$(TARGET)
$(LIBMICROKITCO_3T_AARCH64_OBJ):
	make -f $(LIBMICROKITCO_PATH)/Makefile LIBMICROKITCO_MAX_COTHREADS=3 TARGET=$(TARGET)

$(LIBMICROKITCO_1T_RISCV64_OBJ):
	make -f $(LIBMICROKITCO_PATH)/Makefile LIBMICROKITCO_MAX_COTHREADS=1 TARGET=$(TARGET)
$(LIBMICROKITCO_3T_RISCV64_OBJ):
	make -f $(LIBMICROKITCO_PATH)/Makefile LIBMICROKITCO_MAX_COTHREADS=3 TARGET=$(TARGET)

# Build client system
$(BUILD_DIR)/printf.o: printf.c
	$(CC) $(CFLAGS) -DPRINTF_DISABLE_SUPPORT_FLOAT $^ -o $@

$(BUILD_DIR)/client1.o: client1.c
	$(CC) $(CFLAGS) -I$(LIBMICROKITCO_PATH) $^ -o $@

$(BUILD_DIR)/client2.o: client2.c
	$(CC) $(CFLAGS) -I$(LIBMICROKITCO_PATH) $^ -o $@

$(BUILD_DIR)/client3.o: client3.c
	$(CC) $(CFLAGS) -I$(LIBMICROKITCO_PATH) $^ -o $@

$(BUILD_DIR)/server.o: server.c
	$(CC) $(CFLAGS) -I$(LIBMICROKITCO_PATH) $^ -o $@

# Link everything together
.PHONY: aarch64_link_clients
aarch64_link_clients: $(BUILD_DIR)/client1.o $(BUILD_DIR)/client2.o $(BUILD_DIR)/client3.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_1T_AARCH64_OBJ)
	$(LD) $(LDFLAGS) $(BUILD_DIR)/client1.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_1T_AARCH64_OBJ) $(LIBS) -o $(BUILD_DIR)/client1.elf
	$(LD) $(LDFLAGS) $(BUILD_DIR)/client2.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_1T_AARCH64_OBJ) $(LIBS) -o $(BUILD_DIR)/client2.elf
	$(LD) $(LDFLAGS) $(BUILD_DIR)/client3.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_1T_AARCH64_OBJ) $(LIBS) -o $(BUILD_DIR)/client3.elf
.PHONY: aarch64_link_server
aarch64_link_server:  $(BUILD_DIR)/server.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_3T_AARCH64_OBJ)
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $(BUILD_DIR)/server.elf

.PHONY: riscv64_link_clients
riscv64_link_clients: $(BUILD_DIR)/client1.o $(BUILD_DIR)/client2.o $(BUILD_DIR)/client3.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_1T_RISCV64_OBJ)
	$(LD) $(LDFLAGS) $(BUILD_DIR)/client1.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_1T_RISCV64_OBJ) $(LIBS) -o $(BUILD_DIR)/client1.elf
	$(LD) $(LDFLAGS) $(BUILD_DIR)/client2.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_1T_RISCV64_OBJ) $(LIBS) -o $(BUILD_DIR)/client2.elf
	$(LD) $(LDFLAGS) $(BUILD_DIR)/client3.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_1T_RISCV64_OBJ) $(LIBS) -o $(BUILD_DIR)/client3.elf
.PHONY: riscv64_link_server
riscv64_link_server:  $(BUILD_DIR)/server.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_3T_RISCV64_OBJ)
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $(BUILD_DIR)/server.elf

# Build bootable image
$(AARCH64_TARGET_RULE): aarch64_link_clients aarch64_link_server
	$(MICROKIT_TOOL) codis.system --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(AARCH64_TARGET_RULE) -r $(BUILD_DIR)/report.txt

$(RISCV64_TARGET_RULE): riscv64_link_clients riscv64_link_server
	$(MICROKIT_TOOL) codis.system --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(RISCV64_TARGET_RULE) -r $(BUILD_DIR)/report.txt

# Run on QEMU
run_qemu_aarch64: MICROKIT_BOARD = qemu_arm_virt
run_qemu_aarch64: CPU = cortex-a53
run_qemu_aarch64: CFLAGS += -mtune=$(shell echo $(CPU) | tr A-Z a-z) 
run_qemu_aarch64: clean directories $(AARCH64_TARGET_RULE)
	qemu-system-aarch64 -machine virt,virtualization=on            \
		-cpu "$(CPU)"                                              \
		-serial mon:stdio                                          \
		-device loader,file=$(word 3,$^),addr=0x70000000,cpu-num=0 \
		-m size=2G                                                 \
		-nographic

run_qemu_riscv64: MICROKIT_BOARD = qemu_riscv_virt
run_qemu_riscv64: CPU = medany
run_qemu_riscv64: CFLAGS += -mcmodel=$(shell echo $(CPU) | tr A-Z a-z) -mabi=lp64 -march=rv64imac
run_qemu_riscv64: clean directories $(RISCV64_TARGET_RULE)
	qemu-system-riscv64 -machine virt            \
		-cpu rv64                                                  \
		-serial mon:stdio                                          \
		-kernel $(word 3,$^) \
		-m size=3G                                                 \
		-nographic
