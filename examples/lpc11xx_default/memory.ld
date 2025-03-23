/*
 * Copyright (C) 2022 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

ENTRY(RESET_ISR)
EXTERN(vector_table)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 32K
  RAM (rwx)  : ORIGIN = 0x10000000, LENGTH = 8K - 32

  APB (rw)   : ORIGIN = 0x40000000, LENGTH = 512K
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

    . = ALIGN(4);
    _sidata = .;
  } >FLASH =0xFF

  .data : AT(_sidata) ALIGN(4)
  {
    _sdata = .;

    *(.data)
    *(.data*)

    _edata = .;
  } >RAM

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

  .apb :
  {
    APB_DOMAIN = .;
  } >APB

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
