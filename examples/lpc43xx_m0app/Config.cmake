# Copyright (C) 2024 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set family name
set(FAMILY "LPC")
# Set platform type
set(PLATFORM "LPC43XX_M0APP")

# Available memory regions
math(EXPR ADDRESS_FLASH "0x1B000000")
math(EXPR ADDRESS_SDRAM "0x28000000")
math(EXPR ADDRESS_SPIFI "0x14000000")

# Linker script settings
if(TARGET_NOR)
    set(DFU_LENGTH 131072)
    math(EXPR ROM_LENGTH "4 * 1024 * 1024 - ${DFU_LENGTH}")
    math(EXPR ROM_ORIGIN "${ADDRESS_SPIFI} + ${DFU_LENGTH}")
elseif(TARGET_SDRAM)
    math(EXPR ROM_LENGTH "4 * 1024 * 1024")
    math(EXPR ROM_ORIGIN "${ADDRESS_SDRAM}")
else()
    math(EXPR ROM_LENGTH "256 * 1024")
    math(EXPR ROM_ORIGIN "${ADDRESS_FLASH}")
endif()

# Define template list
set(TEMPLATES_LIST
        serial
        timer
        usb_cdc
)
