# Copyright (C) 2022 xent
# Project is distributed under the terms of the GNU General Public License v3.0

# Set family name
set(FAMILY "LPC")
# Set platform type
set(PLATFORM "LPC43XX")

# Memory regions for all parts
math(EXPR MEMORY_ADDRESS_SDRAM "0x28000000")
math(EXPR MEMORY_ADDRESS_SPIFI "0x14000000")
math(EXPR MEMORY_ADDRESS_SRAM0 "0x10000000")
math(EXPR MEMORY_SIZE_SDRAM "4 * 1024 * 1024")
math(EXPR MEMORY_SIZE_SPIFI "4 * 1024 * 1024")
# Flash-based parts
math(EXPR MEMORY_ADDRESS_FLASH "0x1A000000")
math(EXPR MEMORY_SIZE_FLASH "256 * 1024")
math(EXPR MEMORY_SIZE_SRAM0 "32 * 1024")
# Flash-less parts
math(EXPR MEMORY_SIZE_SRAM0_FLASHLESS "96 * 1024")

# Linker script settings
if(TARGET_NOR)
    if(USE_DFU)
        set(DFU_LENGTH 131072)
    else()
        set(DFU_LENGTH 0)
    endif()

    math(EXPR ROM_LENGTH "${MEMORY_SIZE_SPIFI} - ${DFU_LENGTH}")
    math(EXPR ROM_ORIGIN "${MEMORY_ADDRESS_SPIFI} + ${DFU_LENGTH}")
    set(DISABLE_LITERAL_POOL ON)
elseif(TARGET_SDRAM)
    math(EXPR ROM_LENGTH "${MEMORY_SIZE_SDRAM}")
    math(EXPR ROM_ORIGIN "${MEMORY_ADDRESS_SDRAM}")
    set(DISABLE_LITERAL_POOL ON)
elseif(TARGET_SRAM)
    if("${CMAKE_BUILD_TYPE}" MATCHES "Debug|^$")
        # Use debug builds on flash-less parts only
        math(EXPR ROM_LENGTH "${MEMORY_SIZE_SRAM0_FLASHLESS}")
    else()
        math(EXPR ROM_LENGTH "${MEMORY_SIZE_SRAM0}")
    endif()
    math(EXPR ROM_ORIGIN "${MEMORY_ADDRESS_SRAM0}")
else()
    if(USE_DFU)
        set(DFU_LENGTH 32768)
    else()
        set(DFU_LENGTH 0)
    endif()

    math(EXPR ROM_LENGTH "${MEMORY_SIZE_FLASH} - ${DFU_LENGTH}")
    math(EXPR ROM_ORIGIN "${MEMORY_ADDRESS_FLASH} + ${DFU_LENGTH}")
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
        dma_memcopy
        eeprom
        flash
        i2c
        i2c_slave
        i2s_echo
        i2s_tone
        lifetime:EMULATE=true
        mmcsd
        mock_os
        pin_int
        pm_shutdown
        pm_suspend
        pwm
        rtc
        serial
        serial_dma:SERIAL_DMA_TIMER=true
        spi
        spi_dma=spi:SPI_DMA=true
        spim
        systick
        timer
        timer_alarm=timer:TIMER_SUFFIX=Alarm
        timer_rit=timer:TIMER_SUFFIX=RIT
        timer_sct=timer:TIMER_SUFFIX=SCT
        timer_factory
        usb_cdc
        usb_msc
        usb_uac:UAC_FEEDBACK=true
        wdt
        wdt_timer
        work_queue
        work_queue_unique
)
