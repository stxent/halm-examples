# Copyright (C) 2025 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set family name
set(FAMILY "LPC")
# Set platform type
set(PLATFORM "LPC82X")

# Define template list
set(TEMPLATES_LIST
        adc:ADC_CALIBRATE=true
        adc_oneshot:ADC_CALIBRATE=true
        bod
        dma_memcopy:DMA_TRANSFERS=512
        flash
        i2c
        i2c_dma=i2c:I2C_DMA=true
        pin_int
        pm_sleep
        pwm
        serial
        serial_dma:SERIAL_DMA_TIMER=true
        spi
        spi_dma
        systick
        timer_mrt=timer:TIMER_SUFFIX=MRT
        timer_sct=timer:TIMER_SUFFIX=SCT
        timer_wkt=timer:TIMER_SUFFIX=WKT
        wdt
        wdt_timer
)

if(NOT "${CMAKE_BUILD_TYPE}" MATCHES "Debug|^$")
    list(APPEND TEMPLATES_LIST adc_dma:ADC_CALIBRATE=true)
endif()
