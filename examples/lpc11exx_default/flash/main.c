/*
 * lpc11exx_default/flash/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <halm/pin.h>
#include <halm/platform/nxp/flash.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN PIN(0, 12)
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
  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  struct Interface * const flash = init(Flash, 0);
  assert(flash);

  size_t flashSize, pageSize;
  enum Result res;

  pinSet(led);
  if ((res = ifGetParam(flash, IF_SIZE, &flashSize)) == E_OK)
    pinReset(led);
  assert(res == E_OK);

  pinSet(led);
  if ((res = ifGetParam(flash, IF_FLASH_PAGE_SIZE, &pageSize)) == E_OK)
    pinReset(led);
  assert(res == E_OK);

  uint8_t * const buffer = malloc(pageSize);
  for (size_t i = 0; i < pageSize; ++i)
    buffer[i] = i;

  /* Test sector erase */
  const size_t address = findNearestSector();
  assert(address < flashSize);

  pinSet(led);
  if ((res = ifSetParam(flash, IF_FLASH_ERASE_SECTOR, &address)) == E_OK)
    pinReset(led);
  assert(res == E_OK);

  pinSet(led);
  if ((res = program(flash, buffer, pageSize, address)) == E_OK)
    pinReset(led);
  assert(res == E_OK);

  pinSet(led);
  if ((res = verify(flash, buffer, pageSize, address)) == E_OK)
    pinReset(led);
  assert(res == E_OK);

  /* Page erase is not available on all parts */

  while (1);

  return 0;
}
/*----------------------------------------------------------------------------*/
void __assert_func(const char *file __attribute__((unused)),
    int line __attribute__((unused)),
    const char *func __attribute__((unused)),
    const char *expr __attribute__((unused)))
{
  while (1);
}
