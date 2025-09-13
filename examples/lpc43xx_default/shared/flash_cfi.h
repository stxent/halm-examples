/*
 * lpc43xx_default/shared/flash_cfi.h
 * Copyright (C) 2025 xent
 * Project is distributed under the terms of the MIT License
 */

#ifndef LPC43XX_DEFAULT_SHARED_FLASH_CFI_H_
#define LPC43XX_DEFAULT_SHARED_FLASH_CFI_H_
/*----------------------------------------------------------------------------*/
#include <xcore/interface.h>
#include <stdint.h>
/*----------------------------------------------------------------------------*/
extern const struct InterfaceClass * const FlashCFI;

struct FlashCFIConfig
{
  /** Mandatory: memory mapped area. */
  void *address;
  /** Mandatory: memory size. */
  size_t size;
};

struct FlashCFI
{
  struct Interface base;

  void *address;
  uint32_t offset;
  uint32_t size;
};
/*----------------------------------------------------------------------------*/
#endif /* LPC43XX_DEFAULT_SHARED_FLASH_CFI_H_ */
