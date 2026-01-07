# Copyright (C) 2026 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set family name
set(FAMILY "BOUFFALO")
# Set platform type
set(PLATFORM "BL602")

# Memory regions
math(EXPR DATA_ADDRESS_OFFSET "0x20000000")
math(EXPR MEMORY_ADDRESS_RAM  "0x22020000")
# ITCM, 48 KiB, beginning of the region may be used as an instruction cache
math(EXPR MEMORY_ADDRESS_TCM0 "0x22008000")
# DTCM, 48 KiB
math(EXPR MEMORY_ADDRESS_TCM1 "0x22014000")
math(EXPR MEMORY_ADDRESS_XIP  "0x23000000")
math(EXPR MEMORY_SIZE_RAM "64 * 1024")
math(EXPR MEMORY_SIZE_TCM "48 * 1024")
math(EXPR MEMORY_SIZE_XIP "2 * 1024 * 1024")

# Linker script settings
if(TARGET_SRAM)
    math(EXPR RAM_LENGTH "${MEMORY_SIZE_TCM}")
    math(EXPR RAM_ORIGIN "${MEMORY_ADDRESS_TCM1} + ${DATA_ADDRESS_OFFSET}")
    math(EXPR ROM_LENGTH "${MEMORY_SIZE_RAM}")
    math(EXPR ROM_ORIGIN "${MEMORY_ADDRESS_RAM}")

    set(CONFIG_PLATFORM_BOUFFALO_ICACHE_NONE ON)
    set(CONFIG_PLATFORM_BOUFFALO_ICACHE_32K OFF)
else()
    math(EXPR RAM_LENGTH "${MEMORY_SIZE_RAM}")
    math(EXPR RAM_ORIGIN "${MEMORY_ADDRESS_RAM} + ${DATA_ADDRESS_OFFSET}")
    math(EXPR ROM_LENGTH "${MEMORY_SIZE_XIP}")
    math(EXPR ROM_ORIGIN "${MEMORY_ADDRESS_XIP}")
endif()

# Define template list
set(TEMPLATES_LIST
        lifetime
        machine_timer
        serial
        serial_dma
        timer
        timer_factory
        work_queue
)
