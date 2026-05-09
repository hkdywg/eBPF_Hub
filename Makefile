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

.PHONY: all clean dep_install
all:
clean:
dep_install:

BUILD_TOPDIR := $(shell pwd)

include $(BUILD_TOPDIR)/build_core/definition.mk
include $(BUILD_TOPDIR)/build_core/configure.mk

$(warning $(BUILD_OUTPUT))
# Find all of make.inc
include $(BUILD_TOPDIR)/make.inc

all: $(ALL_TARGETS)
	@echo $(MAKEFILE_LIST)
	@echo $(ALL_TARGETS)
	@echo "Build Done"

clean:
	@rm -rf $(BUILD_OUT_TOPDIR)

dep_install:
	sudo apt update
	sudo apt-get install -y --no-install-recommends \
        libelf1 libelf-dev zlib1g-dev \
        make clang llvm
