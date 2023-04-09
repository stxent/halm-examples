/*
 * lpc43xx_default/atimer/main.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/lpc43xx/atimer.h>
#include <assert.h>
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

  clockEnable(RtcOsc, 0);
  while (!clockReady(RtcOsc));

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, BOARD_LED_INV);

  struct Timer * const timer = init(Atimer, 0);
  assert(timer);

  timerSetOverflow(timer, timerGetFrequency(timer) / 2);
  timerSetCallback(timer, onTimerOverflow, &event);
  timerEnable(timer);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    pinToggle(led);
  }

  return 0;
}
