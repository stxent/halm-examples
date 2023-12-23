# Copyright (C) 2023 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set family name
set(FAMILY "NUMICRO")
# Set platform type
set(PLATFORM "M48X")

# Define template list
set(TEMPLATES_LIST
        adc
        adc_dma
        can
        flash
        i2c
        mmcsd
        pin_int
        pwm_bpwm=pwm:PWM_SUFFIX=BPWM
        serial
        spi
        spi_dma
        spim:SPIM_TIMER=true
        systick
        timer
        usb_cdc
        usb_msc
        wdt
        wdt_timer
)
