/*
 * lpc17xx_default/dac/main.c
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/timer.h>
#include <xcore/interface.h>
#include <xcore/memory.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define VOLTAGE_RANGE 65536
#define VOLTAGE_STEP  4096
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, false);

  struct Interface * const dac = boardSetupDac();

  struct Timer * const timer = boardSetupTimer();
  timerSetOverflow(timer, timerGetFrequency(timer));

  bool event = false;
  timerSetCallback(timer, onTimerOverflow, &event);
  timerEnable(timer);

  unsigned int step = 0;

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    pinSet(led);
    const uint16_t voltage =
        step * (VOLTAGE_RANGE - 1) / (VOLTAGE_RANGE / VOLTAGE_STEP);
    step = step < 16 ? step + 1 : 0;
    ifWrite(dac, &voltage, sizeof(voltage));
    pinReset(led);
  }

  return 0;
}
