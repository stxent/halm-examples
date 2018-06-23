/*
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

ENTRY(RESET_ISR)
EXTERN(vectorTable)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 0x010000 /* 64 KiB */
  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 0x005000 /* 20 KiB */

  OB (rw)    : ORIGIN = 0x1FFFF800, LENGTH = 0x000010 /* 10 bytes */
  APB1 (rw)  : ORIGIN = 0x40000000, LENGTH = 0x010000 /* 64 KiB */
  APB2 (rw)  : ORIGIN = 0x40010000, LENGTH = 0x008000 /* 32 KiB */
  AHB (rw)   : ORIGIN = 0x40018000, LENGTH = 0x018000 /* 96 KiB */
  USB (rw)   : ORIGIN = 0x50000000, LENGTH = 0x040000 /* 256 KiB */
  FSMC (rw)  : ORIGIN = 0xA0000000, LENGTH = 0x000100 /* 256 bytes */
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

  .ob :
  {
    OB_DOMAIN = .;
  } >OB

  .apb1 :
  {
    APB1_DOMAIN = .;
  } >APB1

  .apb2 :
  {
    APB2_DOMAIN = .;
  } >APB2

  .ahb :
  {
    AHB_DOMAIN = .;
  } >AHB

  .usb :
  {
    USB_DOMAIN = .;
  } >USB

  .fsmc :
  {
    FSMC_DOMAIN = .;
  } >FSMC

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
