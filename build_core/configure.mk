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
# BUILD OUT DIRECTORY
###################################################
BUILD_OUTPUT 		:= $(BUILD_TOPDIR)/build_output

CLANG ?= clang
LLVM_STRIP ?= llvm-strip
LIBBPF_SRC := $(BUILD_TOPDIR)/libbpf/src
BPFTOOL_SRC := $(BUILD_TOPDIR)/bpftool/src
BLAZESYM_SRC := $(BUILD_TOPDIR)/blazesym
LIBBPF_OBJ := $(BUILD_OUTPUT)/libbpf.a
BPFTOOL_BUILD_OUTPUT := $(BUILD_OUTPUT)/bpftool
BPFTOOL := $(BPFTOOL_BUILD_OUTPUT)/bootstrap/bpftool
BLAZESYM_BUILD_OUTPUT := $(BUILD_OUTPUT)/blazesym
BLAZESYM_LIB := $(BLAZESYM_BUILD_OUTPUT)/libblazesym_c.a
BLAZESYM_INCLUDE := $(BLAZESYM_SRC)/capi/include
ARCH ?= x86
VMLINUX := $(BUILD_TOPDIR)/vmlinux/$(ARCH)/vmlinux.h

###################################################
# USE_BLAZESYM CONFIGURATION
###################################################

ifdef USE_BLAZESYM
BLAZESYM_CFLAGS := -DUSE_BLAZESYM -I$(BLAZESYM_INCLUDE)
BLAZESYM_LDFLAGS := $(BLAZESYM_LIB) -lrt -ldl -lpthread -lm
else
BLAZESYM_CFLAGS :=
BLAZESYM_LDFLAGS :=
endif

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

# Check cargo for blazesym build (only when USE_BLAZESYM is enabled)
ifdef USE_BLAZESYM
CARGO_CHECK := $(shell cargo --version 2>/dev/null)
ifeq ($(CARGO_CHECK),)
$(error cargo not found. Install Rust toolchain for blazesym build.)
endif
endif

# Use our own libbpf API headers and Linux UAPI headers distributed with
# libbpf to avoid dependency on system-wide headers
INCLUDES := -I$(BUILD_OUTPUT) -I$(BUILD_TOPDIR)/libbpf/include/uapi -I$(dir $(VMLINUX)) $(BLAZESYM_CFLAGS)
CFLAGS := $(CFLAGS_OPT) -Wall -Werror $(EXTRA_CFLAGS) $(LOCAL_CFLAGS)
ALL_LDFLAGS := $(LDFLAGS) $(LDFLAGS_OPT) $(EXTRA_LDFLAGS) $(LOCAL_LDFLAGS) $(BLAZESYM_LDFLAGS)

# Get Clang's default includes on this system.
CLANG_BPF_SYS_INCLUDES = $(shell $(CLANG) -v -E - </dev/null 2>&1 \
		| sed -n '/<...> search starts here:/,/End of search list./{ s| \(/.*\)|-idirafter \1|p  }')

###################################################
# BUILD COMMAND
###################################################

CLEAR_VARS := $(BUILD_TOPDIR)/build_core/clear_vars.mk
BUILD_APP := $(BUILD_TOPDIR)/build_core/build_app.mk

