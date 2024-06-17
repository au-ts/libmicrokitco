ifndef MICROKIT_SDK
$(error your MICROKIT_SDK path is not set.)
endif

ifndef TARGET
$(error your TARGET triple is not set. Required for both LLVM and gcc toolchain setups.)
endif

# Debug so we have serial without an actual serial driver
MICROKIT_CONFIG := debug
BUILD_DIR := build
PWD := $(shell pwd)

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

MICROKIT_TOOL = $(MICROKIT_SDK)/bin/microkit

LIBMICROKITCO_PATH := ../../

CC_INCLUDE_MICROKIT_FLAG = -I$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/include
CFLAGS += -c -g
CFLAGS += -nostdlib -ffreestanding 
CFLAGS += -Wall
CFLAGS += $(CC_INCLUDE_MICROKIT_FLAG) -Iinclude
LDFLAGS = -L$(BOARD_DIR)/lib
LIBS = -lmicrokit -Tmicrokit.ld

# Arch specific obj
LIBMICROKITCO_AARCH64_OBJ := $(BUILD_DIR)/libmicrokitco/libmicrokitco_$(TARGET).o
LIBMICROKITCO_X86_64_OBJ := $(BUILD_DIR)/libmicrokitco/libmicrokitco_$(TARGET).o
LIBMICROKITCO_RISCV64_OBJ := $(BUILD_DIR)/libmicrokitco/libmicrokitco_$(TARGET).o

AARCH64_TARGET_RULE := $(BUILD_DIR)/loader_aarch64.img
X86_64_TARGET_RULE := $(BUILD_DIR)/kernel.elf $(BUILD_DIR)/capdl-initialiser-with-spec.elf
RISCV64_TARGET_RULE := $(BUILD_DIR)/loader_riscv64.img

# Preparation steps
clean:
	rm -rfd build

directories:
	$(info $(shell mkdir -p $(BUILD_DIR)))

# Build client system
$(BUILD_DIR)/printf.o: printf.c
	$(CC) $(CFLAGS) -DPRINTF_DISABLE_SUPPORT_FLOAT $^ -o $@
$(BUILD_DIR)/client.o: client.c
	$(CC) $(CFLAGS) -I$(LIBMICROKITCO_PATH) $^ -o $@

# Build libmicrokitco
LIBMICROKITCO_OPT_PATH := $(PWD)/include
ifndef TOOLCHAIN
LLVM = 1
export LIBMICROKITCO_PATH LIBMICROKITCO_OPT_PATH MICROKIT_SDK BUILD_DIR MICROKIT_BOARD MICROKIT_CONFIG CPU LLVM

else
export LIBMICROKITCO_PATH LIBMICROKITCO_OPT_PATH MICROKIT_SDK BUILD_DIR MICROKIT_BOARD MICROKIT_CONFIG CPU TOOLCHAIN

endif

$(LIBMICROKITCO_AARCH64_OBJ):
	make -f $(LIBMICROKITCO_PATH)/Makefile TARGET=$(TARGET)
$(LIBMICROKITCO_X86_64_OBJ):
	make -f $(LIBMICROKITCO_PATH)/Makefile TARGET=$(TARGET)
$(LIBMICROKITCO_RISCV64_OBJ):
	make -f $(LIBMICROKITCO_PATH)/Makefile TARGET=$(TARGET)

# Link everything together
.PHONY: aarch64_link
aarch64_link: $(BUILD_DIR)/client.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_AARCH64_OBJ)
	$(LD) -L$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/lib $^ $(LIBS) -o $(BUILD_DIR)/client.elf
.PHONY: x86_64_link
x86_64_link: $(BUILD_DIR)/client.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_X86_64_OBJ)
	$(LD) -L$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/lib $^ $(LIBS) -o $(BUILD_DIR)/client.elf
.PHONY: riscv64_link
riscv64_link: $(BUILD_DIR)/client.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_RISCV64_OBJ)
	$(LD) -L$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/lib $^ $(LIBS) -o $(BUILD_DIR)/client.elf

# Build bootable image
$(AARCH64_TARGET_RULE): aarch64_link
	$(MICROKIT_TOOL) simple.system --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $@ -r $(BUILD_DIR)/report.txt
$(X86_64_TARGET_RULE): x86_64_link
	$(MICROKIT_TOOL) simple.system --capdl --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -r $(BUILD_DIR)/report.txt
	$(OBJCP) -O elf32-i386 $(BUILD_DIR)/sel4.elf $(BUILD_DIR)/kernel.elf
$(RISCV64_TARGET_RULE): riscv64_link
	$(MICROKIT_TOOL) simple.system --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $@ -r $(BUILD_DIR)/report.txt

# Run them on QEMU
run_qemu_aarch64: MICROKIT_BOARD = qemu_arm_virt
run_qemu_aarch64: CPU = cortex-a53
run_qemu_aarch64: CFLAGS += -mstrict-align -mtune=$(shell echo $(CPU) | tr A-Z a-z) 
run_qemu_aarch64: clean directories $(AARCH64_TARGET_RULE)
	qemu-system-aarch64 -machine virt,virtualization=on            \
		-cpu "$(CPU)"                                              \
		-serial mon:stdio                                          \
		-device loader,file=$(word 3,$^),addr=0x70000000,cpu-num=0 \
		-m size=2G                                                 \
		-nographic

run_qemu_x86_64: MICROKIT_BOARD = x86_64_virt
run_qemu_x86_64: CPU = Nehalem
run_qemu_x86_64: CFLAGS += -mtune=$(shell echo $(CPU) | tr A-Z a-z) -Wno-parentheses
run_qemu_x86_64: clean directories $(X86_64_TARGET_RULE)
	qemu-system-x86_64                                                                             \
		-cpu $(CPU),-vme,+pdpe1gb,-xsave,-xsaveopt,-xsavec,+fsgsbase,-invpcid,+syscall,+lm,enforce \
		-m "3G"                                                                                    \
		-display none                                                                              \
		-serial mon:stdio                                                                          \
		-kernel $(word 3,$^) \
		-initrd $(word 4,$^)

run_qemu_riscv64: MICROKIT_BOARD = qemu_riscv_virt
run_qemu_riscv64: CPU = medany
run_qemu_riscv64: CFLAGS += -mcmodel=$(shell echo $(CPU) | tr A-Z a-z) -mstrict-align -mabi=lp64 -march=rv64imac
run_qemu_riscv64: clean directories $(RISCV64_TARGET_RULE)
	qemu-system-riscv64 -machine virt            \
		-cpu rv64                                                  \
		-serial mon:stdio                                          \
		-kernel $(word 3,$^) \
		-m size=3G                                                 \
		-nographic
