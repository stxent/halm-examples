/*
 * Copyright (C) 2024 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

ENTRY(RESET_ISR)
EXTERN(vector_table)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 1024K
  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 128K

  APB1 (rw)  : ORIGIN = 0x40000000, LENGTH = 30K
  APB2 (rw)  : ORIGIN = 0x40010000, LENGTH = 19K
  AHB1 (rw)  : ORIGIN = 0x40020000, LENGTH = 384K
  AHB2 (rw)  : ORIGIN = 0x50000000, LENGTH = 387K
  AHB3 (rw)  : ORIGIN = 0xA0000000, LENGTH = 4K
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

  .apb1 :
  {
    APB1_DOMAIN = .;
  } >APB1

  .apb2 :
  {
    APB2_DOMAIN = .;
  } >APB2

  .ahb1 :
  {
    AHB1_DOMAIN = .;
  } >AHB1

  .ahb2 :
  {
    AHB2_DOMAIN = .;
  } >AHB2

  .ahb3 :
  {
    AHB3_DOMAIN = .;
  } >AHB3

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
