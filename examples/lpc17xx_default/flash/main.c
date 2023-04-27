/*
 * lpc17xx_default/flash/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/generic/flash.h>
#include <halm/platform/lpc/flash.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
extern unsigned long _stext;
extern unsigned long _sidata;
extern unsigned long _sdata;
extern unsigned long _edata;
/*----------------------------------------------------------------------------*/
static size_t findNearestSector(void)
{
  static const size_t sectorSize = 4096;

  const size_t textSectionSize = (size_t)(&_sidata - &_stext);
  const size_t roDataSectionSize = (size_t)(&_edata - &_sdata);
  const size_t offset = (textSectionSize + roDataSectionSize)
      * sizeof(unsigned long);

  return ((offset + sectorSize - 1) / sectorSize) * sectorSize;
}
/*----------------------------------------------------------------------------*/
static enum Result program(struct Interface *flash, const uint8_t *buffer,
    size_t length, size_t address)
{
  enum Result res;

  if ((res = ifSetParam(flash, IF_POSITION, &address)) != E_OK)
    return res;

  if (ifWrite(flash, buffer, length) != length)
    return E_INTERFACE;

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static enum Result verify(struct Interface *flash, const uint8_t *pattern,
    size_t length, size_t address)
{
  uint8_t buffer[length];
  enum Result res;

  memset(buffer, 0, length);

  if ((res = ifSetParam(flash, IF_POSITION, &address)) != E_OK)
    return res;

  if (ifRead(flash, buffer, length) != length)
    return E_INTERFACE;

  for (size_t i = 0; i < length; ++i)
  {
    if (buffer[i] != pattern[i])
      return E_VALUE;
  }

  return E_OK;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, BOARD_LED_INV);

  struct Interface * const flash = init(Flash, NULL);
  assert(flash != NULL);

  size_t flashSize, pageSize;
  enum Result res;

  pinWrite(led, !BOARD_LED_INV);
  if ((res = ifGetParam(flash, IF_SIZE, &flashSize)) == E_OK)
    pinWrite(led, BOARD_LED_INV);
  assert(res == E_OK);

  pinWrite(led, !BOARD_LED_INV);
  if ((res = ifGetParam(flash, IF_FLASH_PAGE_SIZE, &pageSize)) == E_OK)
    pinWrite(led, BOARD_LED_INV);
  assert(res == E_OK);

  uint8_t * const buffer = malloc(pageSize);
  for (size_t i = 0; i < pageSize; ++i)
    buffer[i] = i;

  /* Test sector erase */

  const size_t address = findNearestSector();
  assert(address < flashSize);

  pinWrite(led, !BOARD_LED_INV);
  if ((res = ifSetParam(flash, IF_FLASH_ERASE_SECTOR, &address)) == E_OK)
    pinWrite(led, BOARD_LED_INV);
  assert(res == E_OK);

  pinWrite(led, !BOARD_LED_INV);
  if ((res = program(flash, buffer, pageSize, address)) == E_OK)
    pinWrite(led, BOARD_LED_INV);
  assert(res == E_OK);

  pinWrite(led, !BOARD_LED_INV);
  if ((res = verify(flash, buffer, pageSize, address)) == E_OK)
    pinWrite(led, BOARD_LED_INV);
  assert(res == E_OK);

  while (1);
  return 0;
}
