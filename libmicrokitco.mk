#
# Copyright 2025, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause
#
# This Makefile snippet builds the libmicrokitco library.
# The library provides a synchronous API over the asynchronous
# event handlers provided by the seL4 Microkit.
#
# NOTES:
# Generates libmicrokitco.a or libmicrokitco%.a. That is,
# it can generate different versions of the library with
# the same filename stem. This is useful if you want to
# build multiple different versions with different build-
# time configurations. CFLAGS can be configured using the
# LIBMICROKITCO_CFLAGS variable, as well as its suffixed
# variant.
#
# For example, if you only need to generate one version
# of the library, usage is simple:
#
# LIBMICROKITCO_CFLAGS := -O2 -I/path/to/libmicrokitco_opts.h
# include libmicrokitco.mk
# my_program.elf: libmicrokitco.a
#
# However, if you have two different programs requiring
# two differently configured versions of the library, you
# can do this instead:
#
# LIBMICROKITCO_CFLAGS_0 := -O0 -g -I/path/to/program_0/libmicrokitco_opts.h
# LIBMICROKITCO_CFLAGS_1 := -O3 -I/path/to/program_1/libmicrokitco_opts.h
# include libmicrokitco.mk
# program_0.elf: libmicrokitco_0.a
# program_1.elf: libmicrokitco_1.a
#
# Observe that each LIBMICROKITCO_CFLAGS variant should contain the
# include path for the respective libmicrokitco_opts.h.
#
# LIBMICROKITCO_LIBC_INCLUDE can be set to a build target for the C standard
# library include directory if providing your own libc implementation.

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

ifndef LIBCO_PATH
LIBCO_PATH := $(LIBMICROKITCO_PATH)/libco
endif

VARIANTS ?= $(patsubst LIBMICROKITCO_CFLAGS%,%,$(filter LIBMICROKITCO_CFLAGS%,$(.VARIABLES)))

LIBCO_DEPS := \
  $(LIBCO_PATH)/libco.c \
  $(LIBCO_PATH)/libco.h \
  $(LIBCO_PATH)/aarch64.c \
  $(LIBCO_PATH)/amd64.c \
  $(LIBCO_PATH)/arm.c \
  $(LIBCO_PATH)/riscv64.c \
  $(LIBCO_PATH)/settings.h

LIBMICROKITCO_DEPS := \
  $(LIBMICROKITCO_PATH)/libmicrokitco.c \
  $(LIBMICROKITCO_PATH)/libmicrokitco.h \
  $(LIBMICROKITCO_PATH)/libhostedqueue/libhostedqueue.h

define MICROKITCO_RECIPE

LIBDIR$(1) := $(BUILD_DIR)/libmicrokitco$(1)
OBJS$(1) := $$(LIBDIR$(1))/libco.o $$(LIBDIR$(1))/libmicrokitco.o
LIB$(1) := libmicrokitco$(1).a

$$(LIBDIR$(1)):
	mkdir -p $$@

$$(LIBDIR$(1))/libco.o: $$(LIBCO_DEPS) | $$(LIBDIR$(1))
	$$(CC) $$(CFLAGS) $$(LIBMICROKITCO_CFLAGS$(1)) -c -Wno-unused-value $(LIBCO_PATH)/libco.c -o $$@

$$(LIBDIR$(1))/libmicrokitco.o: $$(LIBMICROKITCO_DEPS) | $$(LIBDIR$(1)) $$(LIBMICROKITCO_LIBC_INCLUDE)
	$$(CC) $$(CFLAGS) $$(LIBMICROKITCO_CFLAGS$(1)) -c -I$$(LIBCO_PATH) $(LIBMICROKITCO_PATH)/libmicrokitco.c -o $$@

$$(LIB$(1)): $$(OBJS$(1))
	$$(LD) -r $$^ -o $$@

endef

ifneq ($(VARIANTS),)
$(foreach v,$(VARIANTS),$(eval $(call MICROKITCO_RECIPE,$(v))))
else
$(eval $(call MICROKITCO_RECIPE,))
endif
