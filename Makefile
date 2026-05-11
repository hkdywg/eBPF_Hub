##
## Makefile
##
## History:
##    2025/05/09 - [yinwg] Created file
##
## 	This program is free software; you can redistribute it and/r modify
##  it under the terms of the GNU General Public License version 2 as
##  published by the Free Software Foundation.
##

.PHONY: all clean dep_install help

# Parallel build support
MAKEFLAGS += -j$(shell nproc 2>/dev/null || echo 4)

BUILD_TOPDIR := $(shell pwd)

include $(BUILD_TOPDIR)/build_core/definition.mk
include $(BUILD_TOPDIR)/build_core/configure.mk

# Find all of make.inc
include $(BUILD_TOPDIR)/make.inc

all: $(ALL_TARGETS)
	@echo "Build targets:"
	@for target in $(ALL_TARGETS); do echo "  $$target"; done
	@echo ""
	@echo "Build completed successfully!"

clean:
	@echo "Cleaning build output..."
	@rm -rf $(BUILD_OUTPUT)
	@echo "Clean completed."

dep_install:
	@echo "Installing dependencies..."
	sudo apt update
	sudo apt-get install -y --no-install-recommends \
        libelf1 libelf-dev zlib1g-dev \
        make clang llvm llvm-strip

help:
	@echo "eBPF Build System"
	@echo ""
	@echo "Usage:"
	@echo "  make                  - Build all targets"
	@echo "  make <target>         - Build specific target"
	@echo "  make clean            - Clean build output"
	@echo "  make dep_install      - Install dependencies"
	@echo ""
	@echo "Build Configuration:"
	@echo "  BUILD_TYPE=debug      - Debug build (O0, g3)"
	@echo "  BUILD_TYPE=release    - Release build (O2, g)"
	@echo "  BUILD_TYPE=size       - Size-optimized build (Os)"
	@echo "  STRIP_RELEASE=yes     - Strip release binaries"
	@echo ""
	@echo "Available targets:"
	@for target in $(ALL_TARGETS); do echo "  $$target"; done
