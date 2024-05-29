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

MICROKIT_TOOL = $(MICROKIT_SDK)/bin/microkit

LIBMICROKITCO_PATH := ../../

CC_INCLUDE_MICROKIT_FLAG = -I$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/include
CFLAGS += -c -mstrict-align -nostdlib -ffreestanding -g -Wall -Wno-unused-function -Iinclude $(CC_INCLUDE_MICROKIT_FLAG)
LDFLAGS = -L$(MICROKIT_SDK)/board/$(MICROKIT_BOARD)/$(MICROKIT_CONFIG)/lib
LIBS := -lmicrokit -Tmicrokit.ld

# Our PDs have different needs for number of cothreads so we need multiple configs of the library.
LIBMICROKITCO_3T_AARCH64_OBJ := $(BUILD_DIR)/libmicrokitco/libmicrokitco_3ct_aarch64-none-elf.o
LIBMICROKITCO_4T_AARCH64_OBJ := $(BUILD_DIR)/libmicrokitco/libmicrokitco_4ct_aarch64-none-elf.o
LIBMICROKITCO_3T_RISCV64_OBJ := $(BUILD_DIR)/libmicrokitco/libmicrokitco_3ct_riscv64-none-elf.o
LIBMICROKITCO_4T_RISCV64_OBJ := $(BUILD_DIR)/libmicrokitco/libmicrokitco_4ct_riscv64-none-elf.o

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

$(LIBMICROKITCO_3T_AARCH64_OBJ):
	make -f $(LIBMICROKITCO_PATH)/Makefile LIBMICROKITCO_MAX_COTHREADS=3 TARGET=aarch64-none-elf
$(LIBMICROKITCO_4T_AARCH64_OBJ):
	make -f $(LIBMICROKITCO_PATH)/Makefile LIBMICROKITCO_MAX_COTHREADS=4 TARGET=aarch64-none-elf

$(LIBMICROKITCO_3T_RISCV64_OBJ):
	make -f $(LIBMICROKITCO_PATH)/Makefile LIBMICROKITCO_MAX_COTHREADS=3 TARGET=riscv64-none-elf
$(LIBMICROKITCO_4T_RISCV64_OBJ):
	make -f $(LIBMICROKITCO_PATH)/Makefile LIBMICROKITCO_MAX_COTHREADS=4 TARGET=riscv64-none-elf

# Build client system
$(BUILD_DIR)/printf.o: printf.c
	$(CC) $(CFLAGS) -DPRINTF_DISABLE_SUPPORT_FLOAT $^ -o $@

$(BUILD_DIR)/deadlocked.o: deadlocked.c
	$(CC) $(CFLAGS) -I$(LIBMICROKITCO_PATH) $^ -o $@

$(BUILD_DIR)/deadlock_free.o: deadlock_free.c
	$(CC) $(CFLAGS) -I$(LIBMICROKITCO_PATH) $^ -o $@

$(BUILD_DIR)/many.o: many.c
	$(CC) $(CFLAGS) -I$(LIBMICROKITCO_PATH) $^ -o $@

# Link everything together
.PHONY: aarch64_link_deadlocked
aarch64_link_deadlocked: $(BUILD_DIR)/deadlocked.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_3T_AARCH64_OBJ)
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $(BUILD_DIR)/deadlocked.elf
.PHONY: aarch64_link_deadlock_free
aarch64_link_deadlock_free: $(BUILD_DIR)/deadlock_free.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_3T_AARCH64_OBJ)
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $(BUILD_DIR)/deadlock_free.elf
.PHONY: aarch64_link_many
aarch64_link_many: $(BUILD_DIR)/many.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_4T_AARCH64_OBJ)
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $(BUILD_DIR)/many.elf

.PHONY: riscv64_link_deadlocked
riscv64_link_deadlocked: $(BUILD_DIR)/deadlocked.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_3T_RISCV64_OBJ)
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $(BUILD_DIR)/deadlocked.elf
.PHONY: riscv64_link_deadlock_free
riscv64_link_deadlock_free: $(BUILD_DIR)/deadlock_free.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_3T_RISCV64_OBJ)
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $(BUILD_DIR)/deadlock_free.elf
.PHONY: riscv64_link_many
riscv64_link_many: $(BUILD_DIR)/many.o $(BUILD_DIR)/printf.o $(LIBMICROKITCO_4T_RISCV64_OBJ)
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $(BUILD_DIR)/many.elf

# Build bootable image
$(AARCH64_TARGET_RULE): aarch64_link_deadlocked aarch64_link_deadlock_free aarch64_link_many
	$(MICROKIT_TOOL) join_deadlock.system --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(AARCH64_TARGET_RULE) -r $(BUILD_DIR)/report.txt

$(RISCV64_TARGET_RULE): riscv64_link_deadlocked riscv64_link_deadlock_free riscv64_link_many
	$(MICROKIT_TOOL) join_deadlock.system --search-path $(BUILD_DIR) --board $(MICROKIT_BOARD) --config $(MICROKIT_CONFIG) -o $(RISCV64_TARGET_RULE) -r $(BUILD_DIR)/report.txt

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
	qemu-system-riscv64 -machine virt                              \
		-cpu rv64                                                  \
		-serial mon:stdio                                          \
		-kernel $(word 3,$^)                                       \
		-m size=3G                                                 \
		-nographic
