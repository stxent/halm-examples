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

  pinWrite(leds[0], !BOARD_LED_INV);
  pinToggle(leds[1]);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  boardSetupClockExt();

  struct Pin leds[2] = {
      pinInit(BOARD_LED_0),
      pinInit(BOARD_LED_1)
  };
  pinOutput(leds[0], BOARD_LED_INV);
  pinOutput(leds[1], BOARD_LED_INV);

  struct Interrupt * const bod = init(Bod, &bodConfig);
  assert(bod != NULL);
  interruptSetCallback(bod, onPowerEvent, leds);
  interruptEnable(bod);

  while (1);
  return 0;
}
