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

CC 		:= $(CROSS_COMPILE_CONFIG)gcc
CXX 	:= $(CROSS_COMPILE_CONFIG)g++
AS 		:= $(CROSS_COMPILE_CONFIG)as
LD 		:= $(CROSS_COMPILE_CONFIG)ld
STRIP 	:= $(CROSS_COMPILE_CONFIG)strip
OBJCOPY := $(CROSS_COMPILE_CONFIG)objcopy
OBJDUMP := $(CROSS_COMPILE_CONFIG)objdumo
AR 		:= $(CROSS_COMPILE_CONFIG)ar
NM 		:= $(CROSS_COMPILE_CONFIG)nm


HOST_DIR 		:= $(BUILD_TOPDIR)/out/toolchain/$(TOOL_CHAIN_NAME)
LOCAL_CFLAGS 	:= -I$(HOST_DIR)/include
LOCAL_LDFLAGS 	:= -L$(HOST_DIR)/lib -WL,-rpath,$(HOST_DIR)/lib

ifndef CC
$(error Can not find cross compile toolchain, please source ENV File)
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

# Use our own libbpf API headers and Linux UAPI headers distributed with
# libbpf to avoid dependency on system-wide headers
INCLUDES := -I$(BUILD_OUTPUT) -I$(BUILD_TOPDIR)/libbpf/include/uapi -I$(dir $(VMLINUX))
CFLAGS := -g -Wall
ALL_LDFLAGS := $(LDFLAGS) $(EXTRA_LDFLAGS)

# Get Clang's default includes on this system.
CLANG_BPF_SYS_INCLUDES = $(shell $(CLANG) -v -E - </dev/null 2>&1 \
		| sed -n '/<...> search starts here:/,/End of search list./{ s| \(/.*\)|-idirafter \1|p  }')

###################################################
# BUILD COMMAND
###################################################

CLEAR_VARS := $(BUILD_TOPDIR)/build_core/clear_vars.mk
BUILD_APP := $(BUILD_TOPDIR)/build_core/build_app.mk
BUILD_DRIVER := $(BUILD_TOPDIR)/build_core/build_driver.mk

