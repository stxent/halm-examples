# Copyright (C) 2022 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set family name
set(FAMILY "STM32")
# Set platform type
set(PLATFORM "STM32F0XX")

# Define template list
set(TEMPLATES_LIST
        adc_dma:ADC_CALIBRATE=true
        can
        pin_int
        serial
        serial_dma
        spi
        systick
        timer
        wdt
)
