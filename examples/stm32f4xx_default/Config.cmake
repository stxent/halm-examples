# Copyright (C) 2024 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set family name
set(FAMILY "STM32")
# Set platform type
set(PLATFORM "STM32F4XX")

# Memory regions for all parts
math(EXPR MEMORY_ADDRESS_FLASH "0x08000000")
math(EXPR MEMORY_SIZE_FLASH "512 * 1024")

if(USE_DFU)
    set(DFU_LENGTH 32768)
else()
    set(DFU_LENGTH 0)
endif()

math(EXPR ROM_LENGTH "${MEMORY_SIZE_FLASH} - ${DFU_LENGTH}")
math(EXPR ROM_ORIGIN "${MEMORY_ADDRESS_FLASH} + ${DFU_LENGTH}")

# Define template list
set(TEMPLATES_LIST
        adc
        adc_dma
        can
        dma_memcopy
        flash
        i2c
        i2s_echo
        i2s_tone
        mmcsd
        pin_int
        pm_sleep
        pwm
        serial
        serial_dma
        spi
        systick
        timer
        usb_cdc
        usb_msc
        usb_uac
        wdt
        work_queue
        work_queue_unique
)
