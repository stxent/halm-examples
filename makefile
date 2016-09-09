#Copyright (C) 2016 xent
#Project is distributed under the terms of the GNU General Public License v3.0

CROSS_COMPILE ?= arm-none-eabi-

EXAMPLES_DIR := examples
OUTPUT_DIR := build

ifeq ($(VERBOSE),)
  export Q := @
else
  export Q :=
endif

#TODO
EXAMPLE_FLAGS += CROSS_COMPILE=$(CROSS_COMPILE)
EXAMPLE_FLAGS += XCORE_PATH=../../xcore HALM_PATH=../../halm

EXAMPLE_GROUPS := $(shell find $(EXAMPLES_DIR) -mindepth 1 -maxdepth 1 -type d -printf "%f\n")

LIBS = xcore halm

define make-lib-flags
  $(1)_$(2)_FLAGS += -C $(2)
  $(1)_$(2)_FLAGS += OUTPUT_DIR=../$(OUTPUT_DIR)/$(1)/$(2)
  $(1)_$(2)_FLAGS += CONFIG_FILE=../$(EXAMPLES_DIR)/$(1)/config_$(2)
endef

define make-target-flags
  $(1)_FLAGS += -C $(EXAMPLES_DIR)/$(1)
  $(1)_FLAGS += OUTPUT_DIR=../../$(OUTPUT_DIR)
  $(1)_FLAGS += $(EXAMPLE_FLAGS)
endef

define make-group
  $(1)_LIBS = $(LIBS:%=build_$(1)_%)
  BUILD_TARGETS += build_$(1)
  BUILD_LIBS += $(LIBS:%=build_$(1)_%)
  CLEAN_TARGETS += clean_$(1)
  CLEAN_TARGETS += $(LIBS:%=clean_$(1)_%)

  $(eval $(call make-target-flags,$(1)))
  $(foreach entry,$(LIBS),$(eval $(call make-lib-flags,$(1),$(entry))))
endef

$(foreach entry,$(EXAMPLE_GROUPS),$(eval $(call make-group,$(entry))))

#Define default targets
.PHONY: all clean $(BUILD_TARGETS) $(BUILD_LIBS) $(CLEAN_TARGETS)
.SUFFIXES:
.DEFAULT_GOAL = all

.SECONDEXPANSION:
$(BUILD_TARGETS): $$($$(subst build_,,$$@)_LIBS)
	$(Q)+make --no-print-directory $($(@:build_%=%)_FLAGS)

$(CLEAN_TARGETS):
	$(Q)make --no-print-directory clean $($(@:clean_%=%)_FLAGS)

$(BUILD_LIBS):
	$(Q)+make --no-print-directory $($(@:build_%=%)_FLAGS)

all: $(BUILD_TARGETS)

clean: $(CLEAN_TARGETS)
