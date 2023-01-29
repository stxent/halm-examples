/*
 * lpc43xx_default/wdt/main.c
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/timer.h>
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
  pinSetPull(input, PIN_PULLDOWN);

  const struct Pin failLed = pinInit(BOARD_LED_0);
  pinOutput(failLed, true);
  const struct Pin workLed = pinInit(BOARD_LED_1);
  pinOutput(workLed, true);

  struct Watchdog * const wdt = boardSetupWdt();

  struct Timer * const timer = boardSetupTimer();
  timerSetOverflow(timer, timerGetFrequency(timer) / 10);
  timerSetCallback(timer, onTimerOverflow, &event);
  timerEnable(timer);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    if (pinRead(input))
    {
      watchdogReload(wdt);
      pinReset(failLed);
      pinSet(workLed);
    }
    else
    {
      pinSet(failLed);
      pinReset(workLed);
    }
  }

  return 0;
}
