# Copyright (C) 2022 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set family name
set(FAMILY "LPC")
# Set platform type
set(PLATFORM "LPC17XX")

# Define template configuration
set(TEMPLATES_CONFIG "SERIAL_DMA_TIMER=true")

# Define template list
set(TEMPLATES_LIST
        adc
        adc_dma
        adc_oneshot
        blinking_led
        bod
        can
        clock_out
        dac
        dac_dma
        flash
        gptimer
        i2c
        i2s
        mmcsd
        mmcsd_spi
        pin_int
        serial
        serial_dma
        spi
        spi_dma
        usb_cdc
        usb_msc
        wdt
)
