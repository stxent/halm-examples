/*
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

ENTRY(RESET_ISR)
EXTERN(vector_table)

MEMORY
{
  ROM (rx)    : ORIGIN = ${ROM_ORIGIN}, LENGTH = ${ROM_LENGTH}
  RAM (rwx)   : ORIGIN = 0x20000000, LENGTH = 128K

  CONFIG (r)  : ORIGIN = 0x00300000, LENGTH = 16
  AHB (rw)    : ORIGIN = 0x40000000, LENGTH = 1M
  CRYPTO (rw) : ORIGIN = 0x50000000, LENGTH = 1M
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
  } >ROM =0xFF

  .data : ALIGN(4)
  {
    _sdata = .;

    *(.data)
    *(.data*)

    _edata = .;
  } >RAM AT >ROM

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

  .config :
  {
    CONFIG_DOMAIN = .;
  } >CONFIG

  .ahb :
  {
    AHB_APB_DOMAIN = .;
  } >AHB

  .crypto :
  {
    CRYPTO_DOMAIN = .;
  } >CRYPTO

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
