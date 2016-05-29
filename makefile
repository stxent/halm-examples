#Copyright (C) 2016 xent
#Project is distributed under the terms of the GNU General Public License v3.0

PROJECT_DIR := $(shell pwd)

CROSS_COMPILE ?= arm-none-eabi-

OUTPUT_DIR := $(PROJECT_DIR)/build
EXAMPLES_DIR = $(PROJECT_DIR)/examples

EXAMPLE_GROUPS := $(shell find $(EXAMPLES_DIR) -mindepth 1 -maxdepth 1 -type d -printf "%f\n")
EXAMPLE_FLAGS += CROSS_COMPILE=$(CROSS_COMPILE) OUTPUT_DIR="$(OUTPUT_DIR)"
EXAMPLE_FLAGS += XCORE_PATH="$(PROJECT_DIR)/xcore" HALM_PATH="$(PROJECT_DIR)/halm"
TARGETS += $(EXAMPLE_GROUPS)
CLEAN_TARGETS += $(EXAMPLE_GROUPS:%=clean_%)

$(EXAMPLE_GROUPS):
	@make -C "$(EXAMPLES_DIR)/$@" $(EXAMPLE_FLAGS)

$(CLEAN_TARGETS):
	@make clean -C "$(EXAMPLES_DIR)/$(@:clean_%=%)" $(EXAMPLE_FLAGS)

#Define default targets
.PHONY: all clean $(CLEAN_TARGETS)
.SUFFIXES:
.DEFAULT_GOAL = all

all: $(TARGETS)

clean: $(CLEAN_TARGETS)
