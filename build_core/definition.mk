##
## build_core/definition.mk
##
## History:
##    2026/05/09 - [yinwg] Created file
##
## 	This program is free software; you can redistribute it and/r modify
##  it under the terms of the GNU General Public License version 2 as
##  published by the Free Software Foundation.
##

# Strip quotes and then whitespaces
qstrip = $(strip $(subst ",,$(1)))
#"))

###################################################
# Figure out where we are
###################################################

define my-dir
$(strip \
	$(eval md_file_ := $$(lastword $$(MAKEFILE_LIST))) \
	$(patsubst %/,%,$(dir $(md_file_))) \
	$(eval MAKEFILE_LIST :=  $$(lastword $$(MAKEFILE_LIST))) \
)
endef

###################################################
# Traverse all makefiles in directory
###################################################

define all-makefiles-under
$(wildcard $(1)/*/make.inc)
endef

define all-subdir-makefiles
$(call all-makefiles-under,$(call my-dir))
endef

###################################################
# Add target into ALL_TARGETS
###################################################

define add-target-into-build
$(eval ALL_TARGETS += $(strip $(1)))
endef

###################################################
# Build message output
###################################################

# msg function used in build rules
# Arguments: action, target, optional note
msg = @printf '  %-8s %s\n' "$(1)" "$(2)"

# build-msg for comprehensive logging
define build-msg
@printf ' %-8s %s%s\n'	\
	  "$(1)"					\
	  "$(patsubst $(abspath $(OUTPUT))/%,%,$(2))"	\
	  "$(if $(3), $(3))";
MAKEFLAGS += --no-print-directory
endef

# Quiet make output (Q prefix for commands)
Q ?= @

###################################################
# Download package
###################################################
WGET := wget --passive-ftp -nd -t 3 --no-check-certificate
GIT  := git  

###################################################
# Download package
# Argument 1 is the source location (URL)
# Argument 2 is the package name (for directory naming)
# Argument 3 is the download method (wget/git)
# Argument 4 is optional extra parameters
###################################################
define download-package
	$(Q)mkdir -p $($(2)_DL_DIR)
	$(Q)cd $($(2)_DL_DIR) && \
	if [ "$(3)" = "git" ]; then \
		$(GIT) clone $(4) $(1) $(2); \
	else \
		$(WGET) $(4) $(1); \
	fi
	$(call msg,DOWNLOAD,$(2))
endef





















