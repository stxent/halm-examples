# Copyright (C) 2022 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set family name
set(FAMILY "LPC")
# Set platform type
set(PLATFORM "LPC43XX")

# Define template list
set(TEMPLATES_LIST
        adc
        adc_dma
        adc_oneshot
        bod
        can
        capture
        clock_out
        clock_out_sct=clock_out:COUNTER_SUFFIX=SCT
        counter
        dac
        dac_dma
        eeprom
        flash
        i2c
        i2c_slave
        i2s
        mmcsd
        pin_int
        pm_shutdown
        pm_suspend
        pwm
        rtc
        serial
        serial_dma:SERIAL_DMA_TIMER=true
        spi
        spi_dma
        spim
        systick
        timer
        timer_alarm=timer:TIMER_SUFFIX=Alarm
        timer_rit=timer:TIMER_SUFFIX=RIT
        timer_sct=timer:TIMER_SUFFIX=SCT
        usb_cdc
        usb_msc
        wdt
        wdt_timer
        work_queue
        work_queue_unique
)
