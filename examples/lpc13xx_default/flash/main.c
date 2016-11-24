/*
 * main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <string.h>
#include <halm/pin.h>
#include <halm/platform/nxp/flash.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN     PIN(3, 0)
#define SECTOR_SIZE 4096
/*----------------------------------------------------------------------------*/
extern unsigned long _sdata;
extern unsigned long _edata;
extern unsigned long _sidata;
/*----------------------------------------------------------------------------*/
static enum result program(struct Interface *flash, const uint8_t *buffer,
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
static enum result verify(struct Interface *flash, const uint8_t *pattern,
    size_t length, uint32_t address)
{
  uint8_t buffer[length];
  enum result res;

  memset(buffer, 0, length);

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
  const uint32_t address =
      ((offset + SECTOR_SIZE - 1) / SECTOR_SIZE) * SECTOR_SIZE;

  uint8_t buffer[256];

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

  assert(address < size);

  pinSet(led);
  if ((res = ifSet(flash, IF_FLASH_ERASE_SECTOR, &address)) == E_OK)
    pinReset(led);
  assert(res == E_OK);

  pinSet(led);
  if ((res = program(flash, buffer, sizeof(buffer), address)) == E_OK)
    pinReset(led);
  assert(res == E_OK);

  pinSet(led);
  if ((res = verify(flash, buffer, sizeof(buffer), address)) == E_OK)
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
