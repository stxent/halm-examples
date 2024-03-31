# Copyright (C) 2022 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set family name
set(FAMILY "STM32")
# Set platform type
set(PLATFORM "STM32F1XX")

# Define template list
set(TEMPLATES_LIST
        adc
        adc_dma
        can
        dma_memcopy
        i2c
        mmcsd_spi
        pin_int
        pm_sleep
        serial
        serial_dma
        spi
        systick
        timer
        timer_factory
        usb_cdc
        wdt
        work_queue
)
