/*
 * lpc11exx_default/eeprom/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <string.h>
#include <halm/pin.h>
#include <halm/platform/nxp/eeprom.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN PIN(0, 12)
/*----------------------------------------------------------------------------*/
static enum Result program(struct Interface *eeprom, const uint8_t *buffer,
    size_t length, uint32_t address)
{
  enum Result res;

  if ((res = ifSetParam(eeprom, IF_POSITION, &address)) != E_OK)
    return res;

  if (ifWrite(eeprom, buffer, length) != length)
    return E_INTERFACE;

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static enum Result verify(struct Interface *eeprom, const uint8_t *pattern,
    size_t length, uint32_t address)
{
  uint8_t buffer[length];
  enum Result res;

  memset(buffer, 0, length);

  if ((res = ifSetParam(eeprom, IF_POSITION, &address)) != E_OK)
    return res;

  if (ifRead(eeprom, buffer, length) != length)
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
  static const uint32_t address = 0;
  uint8_t buffer[384];

  for (size_t i = 0; i < sizeof(buffer); ++i)
    buffer[i] = i;

  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  struct Interface * const eeprom = init(Eeprom, 0);
  assert(eeprom);

  uint32_t size;
  enum Result res;

  pinSet(led);
  if ((res = ifGetParam(eeprom, IF_SIZE, &size)) == E_OK)
    pinReset(led);
  assert(res == E_OK);

  assert(address < size);

  pinSet(led);
  if ((res = program(eeprom, buffer, sizeof(buffer), address)) == E_OK)
    pinReset(led);
  assert(res == E_OK);

  pinSet(led);
  if ((res = verify(eeprom, buffer, sizeof(buffer), address)) == E_OK)
    pinReset(led);
  assert(res == E_OK);

  while (1);
  return 0;
}
