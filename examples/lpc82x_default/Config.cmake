# Copyright (C) 2025 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set family name
set(FAMILY "LPC")
# Set platform type
set(PLATFORM "LPC82X")

# Define template list
set(TEMPLATES_LIST
        adc:ADC_CALIBRATE=true
        adc_dma
        adc_oneshot:ADC_CALIBRATE=true
        bod
        flash
        pin_int
        pm_sleep
        pwm
        systick
        timer_mrt=timer:TIMER_SUFFIX=MRT
        timer_sct=timer:TIMER_SUFFIX=SCT
        timer_wkt=timer:TIMER_SUFFIX=WKT
        wdt
        wdt_timer
)
