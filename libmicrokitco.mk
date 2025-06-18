# Copyright 2025, UNSW
# SPDX-License-Identifier: BSD-2-Clause

ifndef MICROKIT_SDK
$(error MICROKIT_SDK is not set)
endif

ifndef BUILD_DIR 
$(error BUILD_DIR is not set)
endif

ifndef MICROKIT_BOARD 
$(error MICROKIT_BOARD is not set)
endif

ifndef MICROKIT_CONFIG 
$(error MICROKIT_CONFIG is not set)
endif

# Absolute path to root directory of libmicrokitco
ifndef LIBMICROKITCO_PATH
$(error LIBMICROKITCO_PATH is not set)
endif

# Absolute path to the directory containing libmicrokitco_opts.h
ifndef LIBMICROKITCO_OPT_PATH
$(error LIBMICROKITCO_OPT_PATH is not set)
endif

ifndef LIBCO_PATH
LIBCO_PATH := $(LIBMICROKITCO_PATH)/libco
endif

$(BUILD_DIR)/libmicrokitco/libco.o: $(LIBCO_PATH)/libco.c $(LIBCO_PATH)/libco.h $(LIBCO_PATH)/aarch64.c $(LIBCO_PATH)/amd64.c $(LIBCO_PATH)/arm.c $(LIBCO_PATH)/riscv64.c $(LIBCO_PATH)/settings.h |$(BUILD_DIR)/libmicrokitco
	$(CC) $(CFLAGS) -c -Wno-unused-value $< -o $@

$(BUILD_DIR)/libmicrokitco/libmicrokitco.o: $(LIBMICROKITCO_PATH)/libmicrokitco.c $(LIBMICROKITCO_PATH)/libmicrokitco.h $(LIBMICROKITCO_PATH)/libhostedqueue/libhostedqueue.h $(LIBMICROKITCO_OPT_PATH)/libmicrokitco_opts.h
	$(CC) $(CFLAGS) -c -I$(LIBCO_PATH) -I$(LIBMICROKITCO_OPT_PATH) $< -o $@

$(BUILD_DIR)/libmicrokitco.a: $(BUILD_DIR)/libmicrokitco/libco.o $(BUILD_DIR)/libmicrokitco/libmicrokitco.o
	$(LD) -r $^ -o $@

$(BUILD_DIR)/libmicrokitco:
	mkdir -p $(BUILD_DIR)/libmicrokitco

clobber::
	rm -f $(BUILD_DIR)/libmicrokitco.a