##
## make.inc
##
## History:
##    2026/05/11 - [yinwg] Created file
##
##  This program is free software; you can redistribute it and/r modify
##  it under the terms of the GNU General Public License version 2 as
##  published by the Free Software Foundation.
##

###########################################################################################################
#   build local linux application
###########################################################################################################

LOCAL_PATH  := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_TARGET    := memleak
LOCAL_SRCS      := $(LOCAL_PATH)/memleak.c \
                   $(LOCAL_PATH)/memleak.bpf.c \
                   $(LOCAL_PATH)/trace_helpers.c \
                   $(LOCAL_PATH)/uprobe_helpers.c

LOCAL_CFLAGS    := -Wall -g
LOCAL_LDFLAGS   := -lelf -lz -lstdc++

# Force g++ linker for C++ ABI functions (__cxa_demangle)
LOCAL_FORCE_CPP_LINK := true

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_MODULE): LOCAL_LDFLAGS := -lelf -lz -lstdc++

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."
ifdef USE_BLAZESYM
	@echo "  (with blazesym support)"
endif

$(call add-target-into-build, $(LOCAL_TARGET))