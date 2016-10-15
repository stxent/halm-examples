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
/*----------------------------------------------------------------------------*/
void writeBlock(struct Interface *flash, const uint8_t *buffer, size_t length,
    uint32_t address)
{
  size_t bytesWritten;
  enum result res;

  res = ifSet(flash, IF_POSITION, &address);
  assert(res == E_OK);

  bytesWritten = ifWrite(flash, buffer, length);
  assert(bytesWritten == length);

  for (size_t i = 0; i < length; ++i)
    assert(((const uint8_t *)address)[i] == buffer[i]);

  /* Suppress warnings */
  (void)bytesWritten;
  (void)res;
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
  pinOutput(led, 0);

  struct Interface * const flash = init(Flash, 0);
  assert(flash);

  uint32_t flashSize;
  enum result res;

  pinSet(led);
  res = ifGet(flash, IF_SIZE, &flashSize);
  assert(res == E_OK);
  assert(sectorAddress < flashSize);
  assert(pageAddress < flashSize);
  pinReset(led);

  pinSet(led);
  res = ifSet(flash, IF_FLASH_ERASE_SECTOR, &sectorAddress);
  assert(res == E_OK);
  pinReset(led);

  pinSet(led);
  writeBlock(flash, buffer, sizeof(buffer), sectorAddress);
  pinReset(led);

  pinSet(led);
  res = ifSet(flash, IF_FLASH_ERASE_PAGE, &pageAddress);
  assert(res == E_OK);
  pinReset(led);

  pinSet(led);
  writeBlock(flash, buffer, sizeof(buffer), pageAddress);
  pinReset(led);

  (void)res; /* Suppress warning */

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
