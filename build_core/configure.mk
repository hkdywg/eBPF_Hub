##
## build_core/configure.mk
##
## History:
##    2026/05/09 - [yinwg] Created file
##
## 	This program is free software; you can redistribute it and/r modify
##  it under the terms of the GNU General Public License version 2 as
##  published by the Free Software Foundation.
##

MAKEFILE_V := 
MAKE_PARA := -s

###################################################
# BUILD TYPE CONFIGURATION
###################################################

# Build type: debug, release, size (default: release)
BUILD_TYPE ?= release

ifeq ($(BUILD_TYPE),debug)
CFLAGS_OPT := -O0 -g3 -DDEBUG
LDFLAGS_OPT :=
else ifeq ($(BUILD_TYPE),release)
CFLAGS_OPT := -O2 -g -DNDEBUG
LDFLAGS_OPT :=
else ifeq ($(BUILD_TYPE),size)
CFLAGS_OPT := -Os -g -DNDEBUG
LDFLAGS_OPT :=
else
$(error Invalid BUILD_TYPE: $(BUILD_TYPE). Valid options: debug, release, size)
endif

# Strip release binaries by default
STRIP_RELEASE ?= yes

###################################################
# TOOLCHAIN CONFIG
###################################################

ifeq ($(CONFIG_LINUX_KERNEL_CROSS_COMPILE),"arm-linux-gnueabi")
TOOL_CHAIN_NAME := $(basename $(basename $(notdir $(CONFIG_ARM_GNUEABI_FULL_NAME))))
CROSS_COMPILE_CONFIG := $(BUILD_TOPDIR)/output/toolchain/$(TOOL_CHAIN_NAME)/bin/arm-linux-gnueabi-
ARCH := arm
CONFIGURE_FLAGS := --target=arm-linux-gnueabi --host=arm-linux-gnueabi --build=x86_64-linux
endif
ifeq ($(CONFIG_LINUX_KERNEL_CROSS_COMPILE),"arm-linux-gnueabihf")
TOOL_CHAIN_NAME := $(basename $(basename $(notdir $(CONFIG_ARM_GNUEABIHF_FULL_NAME))))
CROSS_COMPILE_CONFIG := $(BUILD_TOPDIR)/output/toolchain/$(TOOL_CHAIN_NAME)/bin/arm-linux-gnueabihf-
ARCH := arm
CONFIGURE_FLAGS := --target=arm-linux-gnueabihf --host=arm-linux-gnueabihf --build=x86_64-linux
endif
ifeq ($(CONFIG_LINUX_KERNEL_CROSS_COMPILE),"aarch64-linux-gnu")
TOOL_CHAIN_NAME := $(basename $(basename $(notdir $(CONFIG_AARCH64_FULL_NAME))))
CROSS_COMPILE_CONFIG := $(BUILD_TOPDIR)/output/toolchain/$(TOOL_CHAIN_NAME)/bin/aarch64-linux-gnu-
ARCH := arm64
CONFIGURE_FLAGS := --target=aarch64-linux-gnu --host=aarch64-linux-gnu --build=x86_64-linux
endif

# Default to native compilation if no cross-compile config
ifndef CROSS_COMPILE_CONFIG
CROSS_COMPILE_CONFIG :=
ARCH ?= x86
endif

CC 		:= $(CROSS_COMPILE_CONFIG)gcc
CXX 	:= $(CROSS_COMPILE_CONFIG)g++
AS 		:= $(CROSS_COMPILE_CONFIG)as
LD 		:= $(CROSS_COMPILE_CONFIG)ld
STRIP 	:= $(CROSS_COMPILE_CONFIG)strip
OBJCOPY := $(CROSS_COMPILE_CONFIG)objcopy
OBJDUMP := $(CROSS_COMPILE_CONFIG)objdump
AR 		:= $(CROSS_COMPILE_CONFIG)ar
NM 		:= $(CROSS_COMPILE_CONFIG)nm

ifdef TOOL_CHAIN_NAME
HOST_DIR 		:= $(BUILD_TOPDIR)/out/toolchain/$(TOOL_CHAIN_NAME)
LOCAL_CFLAGS 	:= -I$(HOST_DIR)/include
LOCAL_LDFLAGS 	:= -L$(HOST_DIR)/lib -Wl,-rpath,$(HOST_DIR)/lib
endif

###################################################
# BUILD OUT DIRECTORY
###################################################
BUILD_OUTPUT 		:= $(BUILD_TOPDIR)/build_output

CLANG ?= clang
LLVM_STRIP ?= llvm-strip
LIBBPF_SRC := $(BUILD_TOPDIR)/libbpf/src
BPFTOOL_SRC := $(BUILD_TOPDIR)/bpftool/src
LIBBPF_OBJ := $(BUILD_OUTPUT)/libbpf.a
BPFTOOL_BUILD_OUTPUT := $(BUILD_OUTPUT)/bpftool
BPFTOOL := $(BPFTOOL_BUILD_OUTPUT)/bootstrap/bpftool
ARCH ?= x86
VMLINUX := $(BUILD_TOPDIR)/vmlinux/$(ARCH)/vmlinux.h

###################################################
# TOOL VERSION CHECK
###################################################

# Check minimum clang version (>= 10 required for BPF CO-RE)
CLANG_VERSION := $(shell $(CLANG) --version 2>/dev/null | grep -oE 'clang version [0-9]+' | grep -oE '[0-9]+' | head -1)
ifeq ($(CLANG_VERSION),)
$(error Cannot detect clang version. Please install clang >= 10)
endif
ifneq ($(shell test $(CLANG_VERSION) -ge 10 && echo true),true)
$(error clang version $(CLANG_VERSION) is too old. Minimum required: 10. Install newer clang.)
endif

# Verify LLVM tools are available
LLVM_STRIP_CHECK := $(shell $(LLVM_STRIP) --version 2>/dev/null)
ifeq ($(LLVM_STRIP_CHECK),)
$(error llvm-strip not found. Install llvm package.)
endif

# Use our own libbpf API headers and Linux UAPI headers distributed with
# libbpf to avoid dependency on system-wide headers
INCLUDES := -I$(BUILD_OUTPUT) -I$(BUILD_TOPDIR)/libbpf/include/uapi -I$(dir $(VMLINUX))
CFLAGS := $(CFLAGS_OPT) -Wall -Werror $(EXTRA_CFLAGS) $(LOCAL_CFLAGS)
ALL_LDFLAGS := $(LDFLAGS) $(LDFLAGS_OPT) $(EXTRA_LDFLAGS) $(LOCAL_LDFLAGS)

# Get Clang's default includes on this system.
CLANG_BPF_SYS_INCLUDES = $(shell $(CLANG) -v -E - </dev/null 2>&1 \
		| sed -n '/<...> search starts here:/,/End of search list./{ s| \(/.*\)|-idirafter \1|p  }')

###################################################
# BUILD COMMAND
###################################################

CLEAR_VARS := $(BUILD_TOPDIR)/build_core/clear_vars.mk
BUILD_APP := $(BUILD_TOPDIR)/build_core/build_app.mk
BUILD_DRIVER := $(BUILD_TOPDIR)/build_core/build_driver.mk

