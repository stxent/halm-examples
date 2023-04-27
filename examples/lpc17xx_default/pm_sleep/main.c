/*
 * lpc17xx_default/pm_sleep/main.c
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/pm.h>
#include <halm/timer.h>
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument __attribute__((unused)))
{
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  boardSetupClockExt();

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, BOARD_LED_INV);

  struct Timer * const timer = boardSetupTimer();
  timerSetOverflow(timer, timerGetFrequency(timer) * 5);
  timerSetCallback(timer, onTimerOverflow, NULL);
  timerEnable(timer);

  while (1)
  {
    pmChangeState(PM_SLEEP);
    pinToggle(led);
  }

  return 0;
}
