##
## build_core/build_app.mk
##
## History:
##    2024/04/12 - [yinwg] Created file
##
## 	This program is free software; you can redistribute it and/r modify
##  it under the terms of the GNU General Public License version 2 as
##  published by the Free Software Foundation.
##

$(if $(LOCAL_TARGET),,$(error $(LOCAL_PATH): LOCAL_TARGET is not defined))

# convert bpf.c file to bpf objs
LOCAL_OBJS_BPF := $(filter %.o, $(patsubst $(BUILD_TOPDIR)/%.bpf.c, $(BUILD_OUTPUT)/%.bpf.o, $(LOCAL_SRCS)))

# convert .c file to objs
LOCAL_OBJS_C := $(filter-out %.bpf.o, $(patsubst $(BUILD_TOPDIR)/%.c, $(BUILD_OUTPUT)/%.o, $(LOCAL_SRCS)))

# convert .bpf.o file to skeleton
LOCAL_SKEL_H := $(patsubst %.bpf.o, %.skel.h, $(LOCAL_OBJS_BPF))

# filter local .bpf.c file
LOCAL_BPF_C := $(filter %.bpf.c, $(LOCAL_SRCS))

# filter local .bpf.c file
LOCAL_BASIC_C := $(filter-out %.bpf.c, $(filter %.c, $(LOCAL_SRCS)))

# define variables owned by specific $(LOCAL_LIBS)
PRIVATE_LIBS += $(patsubst lib%.a, -l%, $(filter %.a,$(LOCAL_LIBS)))

# final targets
LOCAL_MODULE := $(patsubst $(BUILD_TOPDIR)/%, $(BUILD_OUTPUT)/%, $(LOCAL_PATH)/$(LOCAL_TARGET))

# final target dir
LOCAL_MODULE_PATH := $(dir $(LOCAL_MODULE))

INCLUDES += -I$(dir $(LOCAL_SKEL_H))

# Skip path-based targets for bash-completion mode
ifeq ($(__BASH_MAKE_COMPLETION__),)

# build output path create
$(LOCAL_MODULE_PATH) $(BUILD_OUTPUT) $(BUILD_OUTPUT)/libbpf $(BPFTOOL_BUILD_OUTPUT):
	$(call msg,MKDIR,$@)
	$(Q)mkdir -p $@

# Build libbpf
$(LIBBPF_OBJ): $(wildcard $(LIBBPF_SRC)/*.[ch] $(LIBBPF_SRC)/Makefile) | $(BUILD_OUTPUT)/libbpf 
	$(call msg,LIB,$@)
	$(Q)$(MAKE) $(MAKEFLAGS) -C $(LIBBPF_SRC) BUILD_STATIC_ONLY=1 \
		OBJDIR=$(dir $@)/libbpf DESTDIR=$(dir $@) \
		INCLUDEDIR= LIBDIR= UAPIDIR= \
		install

# Build bpftool
$(BPFTOOL): | $(BPFTOOL_BUILD_OUTPUT) 
	$(call msg,BPFTOOL,$@)
	$(Q)$(MAKE) $(MAKEFLAGS) ARCH= CROSS_COMPILE= OUTPUT=$(BPFTOOL_BUILD_OUTPUT)/ -C $(BPFTOOL_SRC) bootstrap

# Build BPF code
$(LOCAL_OBJS_BPF): $(LOCAL_MODULE_PATH) $(LOCAL_BPF_C) $(LIBBPF_OBJ)
	$(call msg,BPF,$@)
	$(Q)$(CLANG) -g -O2 -fno-stack-protector -fno-asynchronous-unwind-tables \
		-target bpf -D__TARGET_ARCH_$(ARCH) \
		$(INCLUDES) $(CLANG_BPF_SYS_INCLUDES) \
		-c $(filter %.c,$^) -o $@.tmp
	$(Q)$(LLVM_STRIP) -g $@.tmp -o $@
	$(Q)rm -f $@.tmp 

# Generate BPF skeletons
$(LOCAL_SKEL_H): $(LOCAL_OBJS_BPF) $(BPFTOOL)
	$(call msg,GEN-SKEL,$@)
	$(Q)$(BPFTOOL) gen skeleton $< > $@

# Build user-space code
$(BUILD_OUTPUT)/%.o: $(BUILD_TOPDIR)/%.c $(LOCAL_SKEL_H)
	$(call msg,CC,$@)
	$(Q)cc $(CFLAGS) $(INCLUDES) -c $< -o $@ 

# Build application binary
$(LOCAL_MODULE): $(LOCAL_OBJS_C)  $(LIBBPF_OBJ)
	$(call msg,BINARY,$@)
	$(Q)$(CC) $(CFLAGS) $^ $(ALL_LDFLAGS) -lelf -lz -o $@

# Clean target for this module
.PHONY: clean-$(LOCAL_TARGET)
clean-$(LOCAL_TARGET):
	$(call msg,CLEAN,$(LOCAL_TARGET))
	$(Q)rm -f $(LOCAL_MODULE) $(LOCAL_OBJS_C) $(LOCAL_OBJS_BPF) $(LOCAL_SKEL_H)

# delete failed targets
.DELETE_ON_ERROR:

# keep intermediate (.skel.h, .bpf.o, etc) targets
.SECONDARY:

endif # __BASH_MAKE_COMPLETION__

