/*
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

ENTRY(RESET_ISR)
EXTERN(vectorTable)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 0x08000 /* 32 KiB */
  RAM (rwx)  : ORIGIN = 0x10000000, LENGTH = 0x01FE0 /* 8160 bytes */
}

stackStart = ORIGIN(RAM) + LENGTH(RAM);

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
    heapStart = .;
  } >RAM

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
  PROVIDE(_stack = stackStart);
}
