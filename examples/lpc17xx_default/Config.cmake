# Copyright (C) 2022 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set family name
set(FAMILY "LPC")
# Set platform type
set(PLATFORM "LPC17XX")

# Available memory regions
math(EXPR ADDRESS_FLASH "0x00000000")

# Linker script settings
if(USE_DFU)
    set(DFU_LENGTH 16384)
else()
    set(DFU_LENGTH 0)
endif()

math(EXPR ROM_LENGTH "512 * 1024 - ${DFU_LENGTH}")
math(EXPR ROM_ORIGIN "${ADDRESS_FLASH} + ${DFU_LENGTH}")

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
        dma_memcopy
        flash
        i2c
        i2c_slave
        i2s_echo
        i2s_tone
        lifetime
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
        timer_rit=timer:TIMER_SUFFIX=RIT
        timer_factory
        usb_cdc
        usb_msc
        usb_uac
        wdt
        work_queue
        work_queue_unique
)
