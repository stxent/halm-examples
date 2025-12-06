# Copyright (C) 2023 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set family name
set(FAMILY "NUMICRO")
# Set platform type
set(PLATFORM "M48X")

# Available memory regions
math(EXPR ADDRESS_FLASH "0x00000000")
math(EXPR ADDRESS_SDRAM "0x60000000")
math(EXPR ADDRESS_SPIM  "0x08000000")

# Linker script settings
if(TARGET_NOR)
    if(USE_DFU)
        set(DFU_LENGTH 131072)
    else()
        set(DFU_LENGTH 0)
    endif()

    math(EXPR ROM_LENGTH "4 * 1024 * 1024 - ${DFU_LENGTH}")
    math(EXPR ROM_ORIGIN "${ADDRESS_SPIM} + ${DFU_LENGTH}")
    set(DISABLE_LITERAL_POOL ON)
elseif(TARGET_SDRAM)
    math(EXPR ROM_LENGTH "4 * 1024 * 1024")
    math(EXPR ROM_ORIGIN "${ADDRESS_SDRAM}")
    set(DISABLE_LITERAL_POOL ON)
else()
    if(USE_DFU)
        set(DFU_LENGTH 32768)
    else()
        set(DFU_LENGTH 0)
    endif()

    math(EXPR ROM_LENGTH "256 * 1024 - ${DFU_LENGTH}")
    math(EXPR ROM_ORIGIN "${ADDRESS_FLASH} + ${DFU_LENGTH}")
endif()

# Define template list
set(TEMPLATES_LIST
        adc
        adc_dma
        can
        dma_memcopy
        flash
        i2c
        mmcsd
        pin_int
        pwm_bpwm=pwm:PWM_SUFFIX=BPWM
        serial
        spi
        spi_dma=spi:SPI_DMA=true
        spim:SPIM_TIMER=true
        systick
        timer
        usb_cdc
        usb_msc
        wdt
        wdt_timer
)
