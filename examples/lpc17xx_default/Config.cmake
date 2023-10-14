# Copyright (C) 2022 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set family name
set(FAMILY "LPC")
# Set platform type
set(PLATFORM "LPC17XX")

# Define template list
set(TEMPLATES_LIST
        adc
        adc_dma
        adc_oneshot
        bod
        can
        capture
        clock_out
        counter
        dac
        dac_dma
        flash
        i2c
        i2c_slave
        i2s
        mmcsd
        mmcsd_spi
        pin_int
        pm_shutdown
        pm_sleep
        pm_suspend
        pwm
        rtc
        serial
        serial_dma:SERIAL_DMA_TIMER=true
        spi
        spi_dma
        systick
        timer
        timer_factory
        timer_rit=timer:TIMER_SUFFIX=RIT
        usb_cdc
        usb_msc
        wdt
        work_queue
)
