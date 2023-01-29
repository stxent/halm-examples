/*
 * irq_priorities/main.c
 * Copyright (C) 2015 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/core/cortex/nvic.h>
#include <halm/delay.h>
#include <halm/platform/lpc/gptimer.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
/* #define ENABLE_GROUPING */
/*----------------------------------------------------------------------------*/
static const struct GpTimerConfig lowPriTimerConfig = {
    .frequency = 1000,
    .channel = 0,
    .priority = 1
};

static const struct GpTimerConfig highPriTimerConfig = {
    .frequency = 1000,
    .channel = 1,
    .priority = 3
};
/*----------------------------------------------------------------------------*/
static void lowPriCallback(void *argument)
{
  struct Pin * const pin = argument;

  pinSet(*pin);
  mdelay(1000);
  pinReset(*pin);
}
/*----------------------------------------------------------------------------*/
static void highPriCallback(void *argument)
{
  struct Pin * const pin = argument;

  pinSet(*pin);
  mdelay(100);
  pinReset(*pin);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  boardSetupClockExt();

  struct Pin led[] = {
      pinInit(BOARD_LED_0),
      pinInit(BOARD_LED_1)
  };

  pinOutput(led[0], false);
  pinOutput(led[1], false);

  const uint8_t groups = nvicGetPriorityGrouping();

#ifndef ENABLE_GROUPING
  /* LED's will flash independently */
  nvicSetPriorityGrouping(groups + 1);
#else
  /* Enabling of the led[0] will stop flashing of the led[1] */
  nvicSetPriorityGrouping(groups + 2);
#endif

  struct Timer * const lowPriTimer = init(GpTimer, &lowPriTimerConfig);
  assert(lowPriTimer);
  timerSetOverflow(lowPriTimer, 4000);
  timerSetCallback(lowPriTimer, lowPriCallback, &led[0]);

  struct Timer * const highPriTimer = init(GpTimer, &highPriTimerConfig);
  assert(highPriTimer);
  timerSetOverflow(highPriTimer, 200);
  timerSetCallback(highPriTimer, highPriCallback, &led[1]);

  timerEnable(lowPriTimer);
  timerEnable(highPriTimer);

  while (1);
  return 0;
}
