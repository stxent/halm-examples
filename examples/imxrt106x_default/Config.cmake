# Copyright (C) 2024 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set family name
set(FAMILY "IMXRT")
# Set platform type
set(PLATFORM "IMXRT106X")

# Available memory regions
math(EXPR ADDRESS_FLEXSPI1 "0x60000000")
math(EXPR ADDRESS_FLEXSPI2 "0x70000000")
math(EXPR ADDRESS_ITCM "0x00000000")
math(EXPR ADDRESS_SEMC "0x80000000")

# Linker script settings
if(TARGET_NOR)
    if(USE_DFU)
        set(DFU_LENGTH 131072)
    else()
        set(DFU_LENGTH 0)
    endif()

    math(EXPR ROM_LENGTH "4 * 1024 * 1024 - ${DFU_LENGTH}")
    math(EXPR ROM_ORIGIN "${ADDRESS_FLEXSPI2} + ${DFU_LENGTH}")
elseif(TARGET_SDRAM)
    math(EXPR ROM_LENGTH "4 * 1024 * 1024")
    math(EXPR ROM_ORIGIN "${ADDRESS_SEMC}")
elseif(TARGET_SRAM)
    math(EXPR ROM_LENGTH "128 * 1024")
    math(EXPR ROM_ORIGIN "${ADDRESS_ITCM}")
else()
    if(USE_DFU)
        set(DFU_LENGTH 131072)
    else()
        set(DFU_LENGTH 0)
    endif()

    math(EXPR ROM_LENGTH "2 * 1024 * 1024 - ${DFU_LENGTH}")
    math(EXPR ROM_ORIGIN "${ADDRESS_FLEXSPI1} + ${DFU_LENGTH}")
endif()

# Define template list
set(TEMPLATES_LIST
        dma_memcopy
        lifetime
        serial
        serial_dma
        systick
        timer_pit=timer:TIMER_SUFFIX=PIT
        usb_cdc
)
