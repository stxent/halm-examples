# Copyright (C) 2024 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set family name
set(FAMILY "STM32")
# Set platform type
set(PLATFORM "STM32F4XX")

# Define template list
set(TEMPLATES_LIST
        adc
        adc_dma
        can
        dma_memcopy
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
