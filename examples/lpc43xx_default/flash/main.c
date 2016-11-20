/*
 * main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/pin.h>
#include <halm/platform/nxp/flash.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN     PIN(PORT_6, 6)
#define SECTOR_SIZE 8192
/*----------------------------------------------------------------------------*/
extern unsigned long _sdata;
extern unsigned long _edata;
extern unsigned long _sidata;
extern unsigned long _stext;
/*----------------------------------------------------------------------------*/
enum result program(struct Interface *flash, const uint8_t *buffer,
    size_t length, uint32_t address)
{
  enum result res;

  if ((res = ifSet(flash, IF_POSITION, &address)) != E_OK)
    return res;

  if (ifWrite(flash, buffer, length) != length)
    return E_INTERFACE;

  return E_OK;
}
/*----------------------------------------------------------------------------*/
enum result verify(struct Interface *flash, const uint8_t *pattern,
    size_t length, uint32_t address)
{
  uint8_t buffer[length];
  enum result res;

  if ((res = ifSet(flash, IF_POSITION, &address)) != E_OK)
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
  const uint32_t offset = (uint32_t)(&_sidata + (&_edata - &_sdata));
  const uint32_t sectorAddress =
      ((offset + SECTOR_SIZE - 1) / SECTOR_SIZE) * SECTOR_SIZE;
  const uint32_t pageAddress = sectorAddress + SECTOR_SIZE / 2;

  uint8_t buffer[512];

  for (size_t i = 0; i < sizeof(buffer); ++i)
    buffer[i] = i;

  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  struct Interface * const flash = init(Flash, 0);
  assert(flash);

  uint32_t size;
  enum result res;

  pinSet(led);
  if ((res = ifGet(flash, IF_SIZE, &size)) == E_OK)
    pinReset(led);
  assert(res == E_OK);

  assert(sectorAddress - (uint32_t)&_stext < size);
  assert(pageAddress - (uint32_t)&_stext < size);

  /* Test sector erase */
  pinSet(led);
  if ((res = ifSet(flash, IF_FLASH_ERASE_SECTOR, &sectorAddress)) == E_OK)
    pinReset(led);
  assert(res == E_OK);

  pinSet(led);
  if ((res = program(flash, buffer, sizeof(buffer), sectorAddress)) == E_OK)
    pinReset(led);
  assert(res == E_OK);

  pinSet(led);
  if ((res = verify(flash, buffer, sizeof(buffer), sectorAddress)) == E_OK)
    pinReset(led);
  assert(res == E_OK);

  /* Test page erase */
  pinSet(led);
  if ((res = ifSet(flash, IF_FLASH_ERASE_PAGE, &pageAddress)) == E_OK)
    pinReset(led);
  assert(res == E_OK);

  pinSet(led);
  if ((res = program(flash, buffer, sizeof(buffer), pageAddress)) == E_OK)
    pinReset(led);
  assert(res == E_OK);

  pinSet(led);
  if ((res = verify(flash, buffer, sizeof(buffer), pageAddress)) == E_OK)
    pinReset(led);
  assert(res == E_OK);

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
