/*
 * lpc17xx_default/timer_freerun/main.c
 * Copyright (C) 2026 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/gptimer.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
struct Context
{
  struct Timer *timer;
  struct Pin led;
  uint32_t period;
};
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  struct Context * const context = argument;

  timerSetOverflow(context->timer,
      timerGetValue(context->timer) + context->period);
  pinToggle(context->led);
}
/*----------------------------------------------------------------------------*/
static struct Timer *setupFreerunningTimer(void)
{
  static const struct GpTimerConfig timerConfig = {
      .frequency = 0,
      .channel = 0,
      .freerun = true
  };

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  boardSetupClockExt();

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, BOARD_LED_INV);

  struct Timer * const timer = setupFreerunningTimer();
  struct Context context = {timer, led, clockFrequency(MainClock) / 2};

  timerSetOverflow(timer, context.period);
  timerSetCallback(timer, onTimerOverflow, &context);
  timerEnable(timer);

  while (1);
  return 0;
}
