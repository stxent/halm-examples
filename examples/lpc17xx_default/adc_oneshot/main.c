/*
 * lpc17xx_default/adc_oneshot/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/timer.h>
#include <xcore/interface.h>
#include <xcore/memory.h>
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

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, false);

  struct Interface * const adc = boardSetupAdcOneShot();

  struct Timer * const timer = boardSetupTimer();
  timerSetOverflow(timer, timerGetFrequency(timer));
  timerSetCallback(timer, onTimerOverflow, &event);
  timerEnable(timer);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    pinSet(led);

    uint16_t voltage;
    const size_t bytesRead = ifRead(adc, &voltage, sizeof(voltage));
    assert(bytesRead == sizeof(voltage));
    (void)bytesRead; /* Suppress warning */

    pinReset(led);
  }

  return 0;
}
