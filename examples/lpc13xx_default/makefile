#Copyright (C) 2016 xent
#Project is distributed under the terms of the GNU General Public License v3.0

PROJECT := lpc13xx_default
PROJECT_DIR := $(shell pwd)

CROSS_COMPILE ?= arm-none-eabi-

BUILD_FLAGS += -DLPC13XX
OPT_FLAGS += -Og -g3

#Process build flags
AR := $(CROSS_COMPILE)ar
CC := $(CROSS_COMPILE)gcc
CPU_FLAGS += -mcpu=cortex-m3 -mthumb

#Configure compiler options
CFLAGS += -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections
CFLAGS += -std=c11 -Wall -Wextra -Winline -pedantic -Wshadow -Wcast-qual
CFLAGS += $(OPT_FLAGS) $(CPU_FLAGS) $(BUILD_FLAGS)
LDFLAGS += -nostdlib -Xlinker --gc-sections
LDFLAGS += $(CPU_FLAGS)

#Configure common paths and libraries
INCLUDE_PATH += -Iinclude
OUTPUT_DIR ?= build_$(PROJECT)

#External libraries, pay attention to link order
HALM_PATH ?= $(PROJECT_DIR)/../halm
HALM_OUTPUT_DIR = $(OUTPUT_DIR)/$(PROJECT)/halm
HALM_ARGS += -C $(HALM_PATH) OUTPUT_DIR=$(HALM_OUTPUT_DIR) CONFIG_FILE=$(PROJECT_DIR)/config_halm
HALM_ARGS += XCORE_PATH=$(XCORE_PATH)
INCLUDE_PATH += -I"$(HALM_PATH)/include"
LIBRARY_FILES += $(HALM_OUTPUT_DIR)/libhalm.a

XCORE_PATH ?= $(PROJECT_DIR)/../xcore
XCORE_OUTPUT_DIR = $(OUTPUT_DIR)/$(PROJECT)/xcore
XCORE_ARGS += -C $(XCORE_PATH) OUTPUT_DIR=$(XCORE_OUTPUT_DIR) CONFIG_FILE=$(PROJECT_DIR)/config_xcore
INCLUDE_PATH += -I"$(XCORE_PATH)/include"
LIBRARY_FILES += $(XCORE_OUTPUT_DIR)/libxcore.a

TARGETS += $(LIBRARY_FILES)

#Search for example files
LDFLAGS += -T $(PROJECT_DIR)/memory.ld
EXAMPLES := $(shell find $(PROJECT_DIR) -mindepth 1 -maxdepth 1 -type d -printf "%f\n")
EXAMPLE_FILES = $(EXAMPLES:%=$(OUTPUT_DIR)/$(PROJECT)/%.axf)
TARGETS += $(EXAMPLE_FILES)

define make-example
  $(1)_CSOURCES := $(wildcard $(PROJECT_DIR)/$(1)/*.c)
  CSOURCES += $$($(1)_CSOURCES)
  $(1)_SOURCES := $$($(1)_CSOURCES)
  $(1)_OBJECTS := $$($(1)_SOURCES:%.c=$(OUTPUT_DIR)/$(PROJECT)/%.o)
endef

$(foreach entry,$(EXAMPLES),$(eval $(call make-example,$(entry))))

.SECONDEXPANSION:
$(OUTPUT_DIR)/$(PROJECT)/%.axf: $$(%_OBJECTS) $(LIBRARY_FILES)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ -o $@

COBJECTS = $(CSOURCES:%.c=$(OUTPUT_DIR)/$(PROJECT)/%.o)

#Define default targets
.PHONY: all clean
.SUFFIXES:
.DEFAULT_GOAL = all

all: $(TARGETS)

$(XCORE_OUTPUT_DIR)/libxcore.a:
	@make $(XCORE_ARGS)

$(HALM_OUTPUT_DIR)/libhalm.a:
	@make $(HALM_ARGS)

$(OUTPUT_DIR)/$(PROJECT)/%.o: %.c $(LIBRARY_FILES)
	@mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $(INCLUDE_PATH) -MMD -MF $(@:%.o=%.d) -MT $@ $< -o $@

clean:
	rm -f $(COBJECTS:%.o=%.d) $(COBJECTS)
	rm -f $(TARGETS)
	make clean $(XCORE_ARGS)
	make clean $(HALM_ARGS)

ifneq ($(MAKECMDGOALS),clean)
  -include $(COBJECTS:%.o=%.d)
endif
