# Specify MICROKIT_SDK if your setup is different:
ifndef MICROKIT_SDK
	MICROKIT_SDK := /Users/dreamliner787-9/TS/sdk
endif

# Debug so we have serial without a serial driver
MICROKIT_CONFIG := debug
BUILD_DIR := build

CC = clang
LD = ld.lld
MICROKIT_TOOL = $(MICROKIT_SDK)/bin/microkit

LIBMICROKITCO_PATH := ../../
LIBMICROKITCO_MAX_COTHREADS = 3

CC_INCLUDE_MICROKIT_FLAG = -I$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/include
CFLAGS = -c 
CFLAGS += -nostdlib -ffreestanding 
CFLAGS += -Wall
CFLAGS += $(CC_INCLUDE_MICROKIT_FLAG) -Iinclude
LDFLAGS = -L$(BOARD_DIR)/lib
LIBS = -lmicrokit -Tmicrokit.ld

# Arch specific obj
LIBMICROKITCO_AARCH64_OBJ := $(BUILD_DIR)/libmicrokitco/libmicrokitco_3ct_aarch64-none-elf.o
LIBMICROKITCO_X86_64_OBJ := $(BUILD_DIR)/libmicrokitco/libmicrokitco_3ct_x86_64-none-elf.o
LIBMICROKITCO_RISCV64_OBJ := $(BUILD_DIR)/libmicrokitco/libmicrokitco_3ct_riscv64-none-elf.o

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
export LIBMICROKITCO_PATH MICROKIT_SDK BUILD_DIR MICROKIT_BOARD MICROKIT_CONFIG CPU LIBMICROKITCO_MAX_COTHREADS
$(LIBMICROKITCO_AARCH64_OBJ):
	make -f $(LIBMICROKITCO_PATH)/Makefile TARGET=aarch64-none-elf
$(LIBMICROKITCO_X86_64_OBJ):
	make -f $(LIBMICROKITCO_PATH)/Makefile TARGET=x86_64-none-elf
$(LIBMICROKITCO_RISCV64_OBJ):
	make -f $(LIBMICROKITCO_PATH)/Makefile TARGET=riscv64-none-elf

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
	llvm-objcopy -O elf32-i386 $(BUILD_DIR)/sel4.elf $(BUILD_DIR)/kernel.elf
$(RISCV64_TARGET_RULE): riscv64_link
	$(MICROKIT_TOOL) simple.system --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $@ -r $(BUILD_DIR)/report.txt

# Run them on QEMU
run_qemu_aarch64: MICROKIT_BOARD = qemu_arm_virt
run_qemu_aarch64: CPU = cortex-a53
run_qemu_aarch64: CFLAGS += -target aarch64-none-elf -mstrict-align -mtune=$(shell echo $(CPU) | tr A-Z a-z) 
run_qemu_aarch64: clean directories $(AARCH64_TARGET_RULE)
	qemu-system-aarch64 -machine virt,virtualization=on            \
		-cpu "$(CPU)"                                              \
		-serial mon:stdio                                          \
		-device loader,file=$(word 3,$^),addr=0x70000000,cpu-num=0 \
		-m size=2G                                                 \
		-nographic

run_qemu_x86_64: MICROKIT_BOARD = x86_64_virt
run_qemu_x86_64: CPU = Nehalem
run_qemu_x86_64: CFLAGS += -target x86_64-none-elf -mtune=$(shell echo $(CPU) | tr A-Z a-z) 
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
run_qemu_riscv64: CFLAGS += -target riscv64-none-elf -mcmodel=$(shell echo $(CPU) | tr A-Z a-z) -mstrict-align -mabi=lp64 -march=rv64imac
run_qemu_riscv64: clean directories $(RISCV64_TARGET_RULE)
	qemu-system-riscv64 -machine virt            \
		-cpu rv64                                                  \
		-serial mon:stdio                                          \
		-kernel $(word 3,$^) \
		-m size=3G                                                 \
		-nographic
