/*
 * Copyright (C) 2020 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

ENTRY(RESET_ISR)
EXTERN(vectorTable)

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 64K
  SRAM0 (rwx) : ORIGIN = 0x10000000, LENGTH = 8K - 32
  SRAM1 (rwx) : ORIGIN = 0x20000000, LENGTH = 2K
  SRAM2 (rwx) : ORIGIN = 0x20004000, LENGTH = 2K

  APB (rw)    : ORIGIN = 0x40000000, LENGTH = 512K
  AHB (rw)    : ORIGIN = 0x40080000, LENGTH = 16K
  GPIO (rw)   : ORIGIN = 0x50000000, LENGTH = 16K
  PPB (rw)    : ORIGIN = 0xE0000000, LENGTH = 1M
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
  } >SRAM0

  .bss : ALIGN(4)
  {
    _sbss = .;

    *(.bss)
    *(.bss*)
    *(COMMON)

    _ebss = .;

    . = ALIGN(4);
    heapStart = .;
  } >SRAM0

  .sram1 : ALIGN(4)
  {
    *(.sram1)
    *(.sram1*)
  } >SRAM1

  .sram2 :
  {
    RAM_DOMAIN = .;
  } >SRAM2

  .apb :
  {
    APB_DOMAIN = .;
  } >APB

  .ahb :
  {
    AHB_DOMAIN = .;
  } >AHB

  .gpio :
  {
    GPIO_DOMAIN = .;
  } >GPIO

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

  PROVIDE(end = heapStart);
  PROVIDE(_stack = ORIGIN(SRAM0) + LENGTH(SRAM0));

  PROVIDE(_ssram1 = ORIGIN(SRAM1));
  PROVIDE(_esram1 = ORIGIN(SRAM1) + LENGTH(SRAM1));
}
