/*
 * lpc13uxx_default/clock_out/main.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/lpc/clocking.h>
#include <halm/timer.h>
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

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, BOARD_LED_INV);

  struct Timer * const counterTimer = boardSetupCounterTimer();
  timerEnable(counterTimer);

  struct Timer * const eventTimer = boardSetupTimer();
  timerSetOverflow(eventTimer, timerGetFrequency(eventTimer));
  timerSetCallback(eventTimer, onTimerOverflow, &event);

  boardSetupClockOutput(clockFrequency(MainClock) / 100000);
  const uint32_t CLOCK_FREQUENCY = clockFrequency(ClockOutput);

  /* Start counting */
  timerEnable(eventTimer);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    const uint32_t period = timerGetValue(counterTimer);
    timerSetValue(counterTimer, 0);

    if (period >= CLOCK_FREQUENCY - 1 && period <= CLOCK_FREQUENCY + 1)
      pinWrite(led, BOARD_LED_INV);
    else
      pinWrite(led, !BOARD_LED_INV);
  }

  return 0;
}
