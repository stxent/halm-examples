#Copyright (C) 2016 xent
#Project is distributed under the terms of the GNU General Public License v3.0

PROJECT := lpc13xx_default

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
LDFLAGS += --specs=nosys.specs --specs=nano.specs -Xlinker --gc-sections
LDFLAGS += $(CPU_FLAGS)
LDFLAGS += -T memory.ld

#Configure common paths and libraries
OUTPUT_DIR ?= build_$(PROJECT)

#External libraries, pay attention to link order
XCORE_PATH ?= ../xcore
INCLUDE_PATH += -I"$(XCORE_PATH)/include"
HALM_PATH ?= ../halm
INCLUDE_PATH += -I"$(HALM_PATH)/include"

LIBS += $(OUTPUT_DIR)/$(PROJECT)/halm/libhalm.a
LIBS += $(OUTPUT_DIR)/$(PROJECT)/xcore/libxcore.a

#Search for example files
EXAMPLES := $(shell find . -mindepth 1 -maxdepth 1 -type d -printf "%f\n")
EXAMPLE_FILES = $(EXAMPLES:%=$(OUTPUT_DIR)/$(PROJECT)/%.axf)
TARGETS += $(EXAMPLE_FILES)

define make-example
  $(1)_CSOURCES := $(wildcard $(1)/*.c)
  CSOURCES += $$($(1)_CSOURCES)
  $(1)_SOURCES := $$($(1)_CSOURCES)
  $(1)_OBJECTS := $$($(1)_SOURCES:%.c=$(OUTPUT_DIR)/$(PROJECT)/%.o)
endef

$(foreach entry,$(EXAMPLES),$(eval $(call make-example,$(entry))))

.SECONDEXPANSION:
$(OUTPUT_DIR)/$(PROJECT)/%.axf: $$(%_OBJECTS)
	@echo "$(CC): $@"
	@mkdir -p $(@D)
	$(Q)$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

COBJECTS = $(CSOURCES:%.c=$(OUTPUT_DIR)/$(PROJECT)/%.o)

#Define default targets
.PHONY: all clean
.SECONDARY: $(COBJECTS)
.SUFFIXES:
.DEFAULT_GOAL = all

$(OUTPUT_DIR)/$(PROJECT)/%.o: %.c
	@echo "$(CC): $@"
	@mkdir -p $(@D)
	$(Q)$(CC) -c $(CFLAGS) $(INCLUDE_PATH) -MMD -MF $(@:%.o=%.d) -MT $@ $< -o $@

all: $(TARGETS)

clean:
	rm -f $(COBJECTS:%.o=%.d) $(COBJECTS)
	rm -f $(TARGETS)

ifneq ($(MAKECMDGOALS),clean)
  -include $(COBJECTS:%.o=%.d)
endif
