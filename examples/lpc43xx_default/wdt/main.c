/*
 * lpc43xx_default/wdt/main.c
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/delay.h>
#include <halm/watchdog.h>
/*----------------------------------------------------------------------------*/
int main(void)
{
  boardSetupClockExt();

  const struct Pin input = pinInit(BOARD_BUTTON);
  pinInput(input);
  pinSetPull(input, PIN_PULLDOWN);

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, !BOARD_LED_INV);
  mdelay(100);
  pinToggle(led);

  struct Watchdog * const wdt = boardSetupWdt();

  if (watchdogFired(wdt))
    pinToggle(led);

  while (1)
  {
    if (!pinRead(input))
      watchdogReload(wdt);
  }

  return 0;
}
