/*
 * lpc43xx_default/shared/flash_cfi.c
 * Copyright (C) 2025 xent
 * Project is distributed under the terms of the MIT License
 */

#include "flash_cfi.h"
#include <halm/delay.h>
#include <halm/generic/flash.h>
#include <assert.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
#define BLOCK_SIZE  65536
#define SECTOR_SIZE 4096
/*----------------------------------------------------------------------------*/
static void eraseBlock(uint16_t *, uint32_t);
static void eraseSector(uint16_t *, uint32_t);
static void writeWord(uint16_t *, uint32_t, uint16_t);

static enum Result interfaceInit(void *, const void *);
static enum Result interfaceGetParam(void *, int, void *);
static enum Result interfaceSetParam(void *, int, const void *);
static size_t interfaceRead(void *, void *, size_t);
static size_t interfaceWrite(void *, const void *, size_t);
/*----------------------------------------------------------------------------*/
const struct InterfaceClass * const FlashCFI =
    &(const struct InterfaceClass){
    .size = sizeof(struct FlashCFI),
    .init = interfaceInit,
    .deinit = NULL,

    .setCallback = NULL,
    .getParam = interfaceGetParam,
    .setParam = interfaceSetParam,
    .read = interfaceRead,
    .write = interfaceWrite
};
/*----------------------------------------------------------------------------*/
static void eraseBlock(uint16_t *base, uint32_t offset)
{
  assert(!(offset % BLOCK_SIZE));

  base[0x5555] = 0x00AA;
  base[0x2AAA] = 0x0055;
  base[0x5555] = 0x0080;
  base[0x5555] = 0x00AA;
  base[0x2AAA] = 0x0055;
  base[offset >> 1] = 0x0050;
  mdelay(25);
}
/*----------------------------------------------------------------------------*/
static void eraseSector(uint16_t *base, uint32_t offset)
{
  assert(!(offset % SECTOR_SIZE));

  base[0x5555] = 0x00AA;
  base[0x2AAA] = 0x0055;
  base[0x5555] = 0x0080;
  base[0x5555] = 0x00AA;
  base[0x2AAA] = 0x0055;
  base[offset >> 1] = 0x0030;
  mdelay(25);
}
/*----------------------------------------------------------------------------*/
static void writeWord(uint16_t *base, uint32_t offset, uint16_t value)
{
  assert(!(offset % 2));

  base[0x5555] = 0x00AA;
  base[0x2AAA] = 0x0055;
  base[0x5555] = 0x00A0;
  base[offset >> 1] = value;
  udelay(10);
}
/*----------------------------------------------------------------------------*/
static enum Result interfaceInit(void *object, const void *configBase)
{
  const struct FlashCFIConfig * const config = configBase;
  struct FlashCFI * const interface = object;

  interface->address = config->address;
  interface->offset = 0;
  interface->size = config->size;

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static enum Result interfaceGetParam(void *object, int parameter, void *data)
{
  struct FlashCFI * const interface = object;

  switch ((enum FlashParameter)parameter)
  {
    case IF_FLASH_BLOCK_SIZE:
      *(uint32_t *)data = BLOCK_SIZE;
      return E_OK;

    case IF_FLASH_SECTOR_SIZE:
      *(uint32_t *)data = SECTOR_SIZE;
      return E_OK;

    default:
      break;
  }

  switch ((enum IfParameter)parameter)
  {
    case IF_POSITION:
      *(uint32_t *)data = interface->offset;
      return E_OK;

    case IF_SIZE:
      *(uint32_t *)data = interface->size;
      return E_OK;

    default:
      return E_INVALID;
  }
}
/*----------------------------------------------------------------------------*/
static enum Result interfaceSetParam(void *object, int parameter,
    const void *data)
{
  struct FlashCFI * const interface = object;

  switch ((enum FlashParameter)parameter)
  {
    case IF_FLASH_ERASE_BLOCK:
    {
      const uint32_t position = *(const uint32_t *)data;

      if (position >= interface->size)
        return E_ADDRESS;
      if (position % BLOCK_SIZE)
        return E_ADDRESS;

      eraseBlock(interface->address, position);
      return E_OK;
    }

    case IF_FLASH_ERASE_SECTOR:
    {
      const uint32_t position = *(const uint32_t *)data;

      if (position >= interface->size)
        return E_ADDRESS;
      if (position % SECTOR_SIZE)
        return E_ADDRESS;

      eraseSector(interface->address, position);
      return E_OK;
    }

    default:
      break;
  }

  switch ((enum IfParameter)parameter)
  {
    case IF_POSITION:
    {
      const uint32_t position = *(const uint32_t *)data;

      if (position < interface->size)
      {
        interface->offset = position;
        return E_OK;
      }
      else
        return E_ADDRESS;
    }

    default:
      return E_INVALID;
  }
}
/*----------------------------------------------------------------------------*/
static size_t interfaceRead(void *object, void *buffer, size_t length)
{
  assert(!(length & 1));

  struct FlashCFI * const interface = object;
  const uintptr_t base = (uintptr_t)interface->address;

  if (interface->offset + length > interface->size)
    length = interface->size - interface->offset;

  memcpy(buffer, (void *)(base + interface->offset), length);

  interface->offset += length;
  if (interface->offset == interface->size)
    interface->offset = 0;

  return length;
}
/*----------------------------------------------------------------------------*/
static size_t interfaceWrite(void *object, const void *buffer, size_t length)
{
  assert(!(length & 1));

  struct FlashCFI * const interface = object;
  const uint16_t *input = buffer;

  if (interface->offset + length > interface->size)
    length = interface->size - interface->offset;

  for (size_t index = 0; index < length; index += 2)
    writeWord(interface->address, interface->offset + index, *input++);

  interface->offset += length;
  if (interface->offset == interface->size)
    interface->offset = 0;

  return length;
}
