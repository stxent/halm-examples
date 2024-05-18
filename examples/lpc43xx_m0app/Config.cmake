# Copyright (C) 2024 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set family name
set(FAMILY "LPC")
# Set platform type
set(PLATFORM "LPC43XX_M0APP")

# Memory regions for all parts
math(EXPR MEMORY_ADDRESS_SDRAM "0x28000000")
math(EXPR MEMORY_ADDRESS_SPIFI "0x14000000")
math(EXPR MEMORY_SIZE_SDRAM "4 * 1024 * 1024")
math(EXPR MEMORY_SIZE_SPIFI "4 * 1024 * 1024")
# Flash-based parts
math(EXPR MEMORY_ADDRESS_FLASH "0x1A000000")
math(EXPR MEMORY_SIZE_FLASH "256 * 1024")

# Linker script settings
if(TARGET_NOR)
    set(DFU_LENGTH 131072)
    math(EXPR ROM_LENGTH "${MEMORY_SIZE_SPIFI} - ${DFU_LENGTH}")
    math(EXPR ROM_ORIGIN "${MEMORY_ADDRESS_SPIFI} + ${DFU_LENGTH}")
elseif(TARGET_SDRAM)
    math(EXPR ROM_LENGTH "${MEMORY_SIZE_SDRAM}")
    math(EXPR ROM_ORIGIN "${MEMORY_ADDRESS_SDRAM}")
else()
    math(EXPR ROM_LENGTH "${MEMORY_SIZE_FLASH}")
    math(EXPR ROM_ORIGIN "${MEMORY_ADDRESS_FLASH}")
endif()

# Define template list
set(TEMPLATES_LIST
        serial
        timer
        usb_cdc
)
