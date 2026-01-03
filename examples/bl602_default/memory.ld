/*
 * Copyright (C) 2026 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

ENTRY(start_entry)
EXTERN(trap_trampoline vector_trampoline vector_table)

MEMORY
{
  /* FLASH (rx)   : ORIGIN = 0x21000000, LENGTH = 128K
  RETRAM (rwx) : ORIGIN = 0x40010000, LENGTH = 4K
  TCM0 (rwx)   : ORIGIN = 0x42008000, LENGTH = 48K
  TCM1 (rwx)   : ORIGIN = 0x42014000, LENGTH = 48K
  RAM (rwx)    : ORIGIN = 0x42020000, LENGTH = 64K
  WRAM (rwx)   : ORIGIN = 0x42030000, LENGTH = 112K */

  /* RAM as FLASH */
  FLASH (rx)   : ORIGIN = 0x22020000, LENGTH = 64K
  /* TCM0 as RAM */
  RAM (rwx)    : ORIGIN = 0x42008000, LENGTH = 48K

  RETRAM (rwx) : ORIGIN = 0x40010000, LENGTH = 4K
  TCM1 (rwx)   : ORIGIN = 0x42014000, LENGTH = 48K
  WRAM (rwx)   : ORIGIN = 0x42030000, LENGTH = 112K

  CLIC (rw)    : ORIGIN = 0x02000000, LENGTH = 2004K
  PB (rw)      : ORIGIN = 0x40000000, LENGTH = 64K
}

SECTIONS
{
  .text : ALIGN(4)
  {
    _stext = .;

    *(.startup)
    . = ALIGN(64);
    *(.vectors)
    . = ALIGN(64);
    *(.vectors.trap)

    *(.text)
    *(.text*)
    *(.rodata)
    *(.rodata*)
    *(.srodata)
    *(.srodata*)

    _etext = .;

    . = ALIGN(4);
    _sidata = .;
  } >FLASH =0xFF

  .data : AT(_sidata) ALIGN(4)
  {
    _sdata = .;

    *(.data)
    *(.data*)
    *(.sdata)
    *(.sdata*)

    _edata = .;
  } >RAM

  .bss : ALIGN(4)
  {
    _sbss = .;

    *(.sbss)
    *(.sbss*)
    *(.bss)
    *(.bss*)
    *(COMMON)

    _ebss = .;

    . = ALIGN(4);
    heap_start = .;
  } >RAM

  .clic :
  {
    CLIC_DOMAIN = .;
  } >CLIC

  .pb :
  {
    PB_DOMAIN = .;
  } >PB

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
  PROVIDE(__global_pointer$ = _sbss + 0x800);
  PROVIDE(_stack = ORIGIN(RAM) + LENGTH(RAM));
}
