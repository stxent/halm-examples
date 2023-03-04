/*
 * lpc43xx_default/eeprom/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/lpc/eeprom.h>
#include <assert.h>
#include <string.h>
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
  static const uint32_t EEPROM_ADDRESS = 64;
  uint8_t buffer[384];

  boardSetupClockExt();

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, false);

  struct Interface * const eeprom = init(Eeprom, 0);
  assert(eeprom);

  uint32_t capacity;
  enum Result res;

  pinWrite(led, !BOARD_LED_INV);
  if ((res = ifGetParam(eeprom, IF_SIZE, &capacity)) == E_OK)
    pinWrite(led, BOARD_LED_INV);
  assert(res == E_OK);
  assert(EEPROM_ADDRESS < capacity);

  /* Fill memory with pseudorandom pattern */

  for (size_t i = 0; i < sizeof(buffer); ++i)
    buffer[i] = i;

  pinWrite(led, !BOARD_LED_INV);
  if ((res = program(eeprom, buffer, sizeof(buffer), EEPROM_ADDRESS)) == E_OK)
    pinWrite(led, BOARD_LED_INV);
  assert(res == E_OK);

  pinWrite(led, !BOARD_LED_INV);
  if ((res = verify(eeprom, buffer, sizeof(buffer), EEPROM_ADDRESS)) == E_OK)
    pinWrite(led, BOARD_LED_INV);
  assert(res == E_OK);

  /* Reset memory to the default state */

  memset(buffer, 0, sizeof(buffer));

  pinWrite(led, !BOARD_LED_INV);
  if ((res = program(eeprom, buffer, sizeof(buffer), EEPROM_ADDRESS)) == E_OK)
    pinWrite(led, BOARD_LED_INV);
  assert(res == E_OK);

  pinWrite(led, !BOARD_LED_INV);
  if ((res = verify(eeprom, buffer, sizeof(buffer), EEPROM_ADDRESS)) == E_OK)
    pinWrite(led, BOARD_LED_INV);
  assert(res == E_OK);

  while (1);
  return 0;
}
