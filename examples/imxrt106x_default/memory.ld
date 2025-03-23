/*
 * Copyright (C) 2024 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

ENTRY(RESET_ISR)
EXTERN(vector_table)

MEMORY
{
  /*ROM (rx)    : ORIGIN = ${ROM_ORIGIN}, LENGTH = ${ROM_LENGTH}*/
  ROM (rx)    : ORIGIN = 0x60000000, LENGTH = 2M
  RAM (rwx)   : ORIGIN = 0x20200000, LENGTH = 512K

  AIPS1 (rw)  : ORIGIN = 0x40000000, LENGTH = 1M
  AIPS2 (rw)  : ORIGIN = 0x40100000, LENGTH = 1M
  AIPS3 (rw)  : ORIGIN = 0x40200000, LENGTH = 1M
  AIPS4 (rw)  : ORIGIN = 0x40300000, LENGTH = 1M
  AIPS5 (rw)  : ORIGIN = 0x42000000, LENGTH = 1M
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
  } >ROM =0xFF

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

  .aips1 :
  {
    AIPS1_DOMAIN = .;
  } >AIPS1

  .aips2 :
  {
    AIPS2_DOMAIN = .;
  } >AIPS2

  .aips3 :
  {
    AIPS3_DOMAIN = .;
  } >AIPS3

  .aips4 :
  {
    AIPS4_DOMAIN = .;
  } >AIPS4

  .aips5 :
  {
    AIPS5_DOMAIN = .;
  } >AIPS5

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
