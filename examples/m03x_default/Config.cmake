# Copyright (C) 2023 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set family name
set(FAMILY "NUMICRO")
# Set platform type
set(PLATFORM "M03X")

# Define template list
set(TEMPLATES_LIST
        adc:ADC_CALIBRATE=true
        adc_dma:ADC_CALIBRATE=true
        flash
        i2c
        pin_int
        pm_sleep
        pwm_bpwm=pwm:PWM_SUFFIX=BPWM
        serial
        serial_dma:SERIAL_DMA_TIMER=true
        spi
        spi_dma
        systick
        timer
        timer_factory
        usb_cdc
        wdt
        work_queue
)
