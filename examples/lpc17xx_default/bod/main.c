/*
 * lpc17xx_default/bod/main.c
 * Copyright (C) 2018 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/lpc/bod.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
static const struct BodConfig bodConfig = {
    .eventLevel = BOD_EVENT_2V2,
    .resetLevel = BOD_RESET_DISABLED
};
/*----------------------------------------------------------------------------*/
static void onPowerEvent(void *argument)
{
  struct Pin * const leds = argument;

  pinSet(leds[0]);
  pinToggle(leds[1]);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  struct Pin leds[2] = {
      pinInit(BOARD_LED_0),
      pinInit(BOARD_LED_1)
  };
  pinOutput(leds[0], false);
  pinOutput(leds[1], false);

  struct Interrupt * const bod = init(Bod, &bodConfig);
  assert(bod);
  interruptSetCallback(bod, onPowerEvent, leds);
  interruptEnable(bod);

  while (1);
  return 0;
}
