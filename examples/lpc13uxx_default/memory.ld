/*
 * Copyright (C) 2020 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

ENTRY(RESET_ISR)
EXTERN(vectorTable)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 0x008000 /* 32 KiB */
  RAM (rwx)  : ORIGIN = 0x10000000, LENGTH = 0x001FE0 /* 8160 bytes */

  APB (rw)   : ORIGIN = 0x40000000, LENGTH = 0x080000 /* 512 KiB */
  AHB (rw)   : ORIGIN = 0x48000000, LENGTH = 0x004000 /* 16 KiB */
  GPIO (rw)  : ORIGIN = 0x50000000, LENGTH = 0x004000 /* 16 KiB */
  PPB (rw)   : ORIGIN = 0xE0000000, LENGTH = 0x100000 /* 1 MiB */
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
    heapStart = .;
  } >RAM

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
  PROVIDE(_stack = ORIGIN(RAM) + LENGTH(RAM));
}
