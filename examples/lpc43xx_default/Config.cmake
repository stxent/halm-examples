# Copyright (C) 2022 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set family name
set(FAMILY "LPC")
# Set platform type
set(PLATFORM "LPC43XX")

# Available memory regions
math(EXPR ADDRESS_FLASH "0x1A000000")
math(EXPR ADDRESS_SDRAM "0x28000000")
math(EXPR ADDRESS_SPIFI "0x14000000")
math(EXPR ADDRESS_SRAM0 "0x10000000")

# Linker script settings
if(TARGET_NOR)
    if(USE_DFU)
        set(DFU_LENGTH 131072)
    else()
        set(DFU_LENGTH 0)
    endif()

    math(EXPR ROM_LENGTH "4 * 1024 * 1024 - ${DFU_LENGTH}")
    math(EXPR ROM_ORIGIN "${ADDRESS_SPIFI} + ${DFU_LENGTH}")
elseif(TARGET_SDRAM)
    math(EXPR ROM_LENGTH "4 * 1024 * 1024")
    math(EXPR ROM_ORIGIN "${ADDRESS_SDRAM}")
elseif(TARGET_SRAM)
    math(EXPR ROM_LENGTH "96 * 1024")
    math(EXPR ROM_ORIGIN "${ADDRESS_SRAM0}")
else()
    if(USE_DFU)
        set(DFU_LENGTH 32768)
    else()
        set(DFU_LENGTH 0)
    endif()

    math(EXPR ROM_LENGTH "256 * 1024 - ${DFU_LENGTH}")
    math(EXPR ROM_ORIGIN "${ADDRESS_FLASH} + ${DFU_LENGTH}")
endif()

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
