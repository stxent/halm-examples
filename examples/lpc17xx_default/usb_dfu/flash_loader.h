/*
 * lpc17xx_default/usb_dfu/flash_loader.h
 * Copyright (C) 2018 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

 #ifndef LPC17XX_DEFAULT_USB_DFU_FLASH_LOADER_H_
 #define LPC17XX_DEFAULT_USB_DFU_FLASH_LOADER_H_

#include <halm/usb/dfu.h>
/*----------------------------------------------------------------------------*/
extern const struct EntityClass * const FlashLoader;

struct FlashGeometry
{
  /** Sector count in a flash region. */
  size_t count;
  /** Size of each sector in a region. */
  size_t size;
  /** Sector erase time in milliseconds. */
  uint32_t time;
};

struct FlashLoaderConfig
{
  /** Mandatory: geometry of the flash memory. */
  const struct FlashGeometry *geometry;
  /** Mandatory: flash memory interface. */
  struct Interface *flash;
  /** Mandatory: DFU instance. */
  struct Dfu *device;
  /** Mandatory: offset from the beginning of the flash memory. */
  size_t offset;
};

struct FlashLoader
{
  struct Entity base;

  struct Interface *flash;
  struct Dfu *device;

  const struct FlashGeometry *geometry;
  uint8_t *chunk;

  size_t flashOffset;
  size_t flashSize;
  size_t pageSize;

  size_t bufferSize;
  size_t currentPosition;

  size_t erasingPosition;
  bool eraseQueued;
};
/*----------------------------------------------------------------------------*/
#endif // LPC17XX_DEFAULT_USB_DFU_FLASH_LOADER_H_
