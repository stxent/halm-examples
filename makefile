#Copyright (C) 2016 xent
#Project is distributed under the terms of the GNU General Public License v3.0

PROJECT_DIR := $(shell pwd)

CROSS_COMPILE ?= arm-none-eabi-

OUTPUT_DIR := $(PROJECT_DIR)/build
EXAMPLES_DIR = $(PROJECT_DIR)/examples

EXAMPLE_GROUPS := $(shell find $(EXAMPLES_DIR) -mindepth 1 -maxdepth 1 -type d -printf "%f\n")
TARGETS += $(EXAMPLE_GROUPS)

$(EXAMPLE_GROUPS):
	@make -C $(EXAMPLES_DIR)/$@ CROSS_COMPILE=$(CROSS_COMPILE) OUTPUT_DIR=$(OUTPUT_DIR) XCORE_PATH=$(PROJECT_DIR)/xcore HALM_PATH=$(PROJECT_DIR)/halm

#Define default targets
.PHONY: all clean
.SUFFIXES:
.DEFAULT_GOAL = all

all: $(TARGETS)

clean:
	rm -rf $(OUTPUT_DIR)
