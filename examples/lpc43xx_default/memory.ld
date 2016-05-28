GROUP(libcr_nohost.a libcr_c.a libcr_eabihelpers.a)

MEMORY
{
  Flash (rx) : ORIGIN = 0x1A000000, LENGTH = 0x40000 /* 256 KiB */
  Ram (rwx) : ORIGIN = 0x10000000, LENGTH = 0x7FE0 /* 32736 bytes */
}

__top_Flash = 0x0 + 0x80000;
__top_Ram = 0x10000000 + 0x7FE0;

EXTERN(vectorTable)
ENTRY(RESET_ISR)

SECTIONS
{
  .text : ALIGN(4)
  {
    FILL(0xFF)
    *(.vectors)
    *(.text*)
    . = ALIGN(4);
    *(.rodata .rodata.*)
    . = ALIGN(4);
  } > Flash

  _etext = .;

  .uninit_RESERVED : ALIGN(4)
  {
    KEEP(*(.bss.$RESERVED*))
    . = ALIGN(4);
    _end_uninit_RESERVED = .;
  } > Ram

  .data : ALIGN(4)
  {
    FILL(0xFF)
    _data = .;
    *(vtable)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > Ram AT>Flash

  .bss : ALIGN(4)
  {
    _bss = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    PROVIDE(end = .);
  } > Ram

  .noinit (NOLOAD) : ALIGN(4)
  {
    _noinit = .;
    *(.noinit*)
    . = ALIGN(4);
    _end_noinit = .;
  } > Ram

  PROVIDE(_pvHeapStart = .);
  PROVIDE(_stack = __top_Ram);
}
