/*
 * lpc17xx_default/i2c_slave/main.c
 * Copyright (C) 2013 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/lpc/i2c_slave.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
static void onDeviceMemoryChanged(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  static const uint32_t DEFAULT_OFFSET = 0;
  static const uint32_t DEVICE_ADDRESS = 0x60;

  bool event = false;
  enum Result res;

  boardSetupClockPll();

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, BOARD_LED_INV);

  struct Interface * const i2c = boardSetupI2CSlave();
  ifSetCallback(i2c, onDeviceMemoryChanged, &event);

  res = ifSetParam(i2c, IF_ADDRESS, &DEVICE_ADDRESS);
  assert(res == E_OK);
  (void)res; /* Suppress warning */

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    uint8_t state;

    ifSetParam(i2c, IF_POSITION, &DEFAULT_OFFSET);
    ifRead(i2c, &state, sizeof(state));
    pinWrite(led, (state & 1) ? !BOARD_LED_INV : BOARD_LED_INV);
  }

  return 0;
}
