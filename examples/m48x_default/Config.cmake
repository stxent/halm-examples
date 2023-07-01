# Copyright (C) 2023 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set family name
set(FAMILY "NUMICRO")
# Set platform type
set(PLATFORM "M48X")

# Define template configuration
set(TEMPLATES_CONFIG "SERIAL_DMA_TIMER=true")

# Define template list
set(TEMPLATES_LIST
        adc
        adc_dma
        blinking_led
        can
        flash
        gptimer
        i2c
        mmcsd
        pin_int
        serial
        spi
        spi_dma
        usb_cdc
        usb_msc
        wdt
)
