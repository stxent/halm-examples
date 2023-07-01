# Copyright (C) 2023 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set family name
set(FAMILY "NUMICRO")
# Set platform type
set(PLATFORM "M03X")

# Define template configuration
set(TEMPLATES_CONFIG "ADC_CALIBRATE=true,SERIAL_DMA_TIMER=true")

# Define template list
set(TEMPLATES_LIST
        adc
        adc_dma
        blinking_led
        flash
        gptimer
        i2c
        pin_int
        serial
        serial_dma
        spi
        spi_dma
        usb_cdc
        wdt
        work_queue
)
