# Copyright (C) 2022 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set family name
set(FAMILY "LPC")
# Set platform type
set(PLATFORM "LPC13UXX")

# Define template list
set(TEMPLATES_LIST
        adc
        adc_oneshot
        bod
        capture
        clock_out
        counter
        eeprom
        flash
        i2c
        pin_int
        pwm
        serial
        spi
        systick
        timer
        timer_rit=timer:TIMER_SUFFIX=RIT
        usb_cdc
        wdt
        wdt_timer
)
