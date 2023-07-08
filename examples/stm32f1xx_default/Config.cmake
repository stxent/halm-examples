# Copyright (C) 2022 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set family name
set(FAMILY "STM32")
# Set platform type
set(PLATFORM "STM32F1XX")

# Define template list
set(TEMPLATES_LIST
        can
        mmcsd_spi
        pin_int
        pm_sleep
        serial
        serial_dma
        software_timer
        spi
        systick
        timer
        usb_cdc
        wdt
        work_queue
)
