/*
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

ENTRY(RESET_ISR)
EXTERN(vector_table)

MEMORY
{
  FLASH (rx) : ORIGIN = ${ROM_ORIGIN}, LENGTH = ${ROM_LENGTH}
  RAM (rwx)  : ORIGIN = 0x10000000, LENGTH = 32K

  GPIO (rw)  : ORIGIN = 0x2009C000, LENGTH = 16K
  APB0 (rw)  : ORIGIN = 0x40000000, LENGTH = 512K
  APB1 (rw)  : ORIGIN = 0x40080000, LENGTH = 512K
  AHB (rw)   : ORIGIN = 0x50000000, LENGTH = 2M
  PPB (rw)   : ORIGIN = 0xE0000000, LENGTH = 1M
}

SECTIONS
{
  .text : ALIGN(4)
  {
    _stext = .;

    *(.vectors)
    *(.text)
    *(.text*)
    *(.rodata)
    *(.rodata*)

    _etext = .;
  } >FLASH =0xFF

  .data : ALIGN(4)
  {
    _sdata = .;

    *(.data)
    *(.data*)

    _edata = .;
  } >RAM AT >FLASH

  _sidata = LOADADDR(.data);

  .bss : ALIGN(4)
  {
    _sbss = .;

    *(.bss)
    *(.bss*)
    *(COMMON)

    _ebss = .;

    . = ALIGN(4);
    heap_start = .;
  } >RAM

  .gpio :
  {
    GPIO_DOMAIN = .;
  } >GPIO

  .apb0 :
  {
    APB0_DOMAIN = .;
  } >APB0

  .apb1 :
  {
    APB1_DOMAIN = .;
  } >APB1

  .ahb :
  {
    AHB_DOMAIN = .;
  } >AHB

  .ppb :
  {
    PPB_DOMAIN = .;
  } >PPB

  /DISCARD/ :
  {
    *(.init)
    *(.fini)

    *(.ARM.exidx*)
    *(.gnu.linkonce.armexidx.*)

    *(.ARM.extab*)
    *(.gnu.linkonce.armextab.*)
  }

  PROVIDE(end = heap_start);
  PROVIDE(_stack = ORIGIN(RAM) + LENGTH(RAM));
}
