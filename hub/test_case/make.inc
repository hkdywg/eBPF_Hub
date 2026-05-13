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

LOCAL_PATH := $(call my-dir)

LEAK_TEST_DIR := $(BUILD_OUTPUT)/hub/test_case
LEAK_TEST_SRC := $(LOCAL_PATH)/leak_test.c
LEAK_TEST_CPP_SRC := $(LOCAL_PATH)/leak_test_cpp.cpp
LEAK_TEST_BIN := $(LEAK_TEST_DIR)/leak_test
LEAK_TEST_CPP_BIN := $(LEAK_TEST_DIR)/leak_test_cpp

.PHONY: leak_test leak_test_cpp

$(LEAK_TEST_DIR):
	@mkdir -p $@

$(LEAK_TEST_BIN): $(LEAK_TEST_SRC) | $(LEAK_TEST_DIR)
	@echo "  CC      $@"
	@cc -Wall -g -fno-omit-frame-pointer -o $@ $<

$(LEAK_TEST_CPP_BIN): $(LEAK_TEST_CPP_SRC) | $(LEAK_TEST_DIR)
	@echo "  CXX     $@"
	@g++ -Wall -g -fno-omit-frame-pointer -static-libstdc++ -std=c++11 -o $@ $<

leak_test: $(LEAK_TEST_BIN)
	@echo "Build $@ Done."

leak_test_cpp: $(LEAK_TEST_CPP_BIN)
	@echo "Build $@ Done."

$(call add-target-into-build, leak_test)
$(call add-target-into-build, leak_test_cpp)