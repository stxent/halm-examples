/*
 * lpc13uxx_default/wwdt/main.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/delay.h>
#include <halm/watchdog.h>
#include <xcore/memory.h>
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  bool event = false;

  boardSetupClockExt();

  const struct Pin input = pinInit(BOARD_BUTTON);
  pinInput(input);
  pinSetPull(input, PIN_PULLUP);

  const struct Pin feedLed = pinInit(BOARD_LED_1);
  pinOutput(feedLed, BOARD_LED_INV);
  const struct Pin failLed = pinInit(BOARD_LED_0);
  pinOutput(failLed, !BOARD_LED_INV);
  mdelay(100);
  pinToggle(failLed);

  struct Watchdog * const wdt = boardSetupWwdt();
  watchdogSetCallback(wdt, onTimerOverflow, &event);

  if (watchdogFired(wdt))
    pinToggle(failLed);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    if (!pinRead(input))
    {
      watchdogReload(wdt);

      pinWrite(feedLed, !BOARD_LED_INV);
      mdelay(100);
      pinWrite(feedLed, BOARD_LED_INV);
    }
  }

  return 0;
}
