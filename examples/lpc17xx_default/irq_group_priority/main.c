/*
 * irq_group_priority/main.c
 * Copyright (C) 2015 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/core/cortex/nvic.h>
#include <halm/delay.h>
#include <halm/platform/lpc/gptimer.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
static const struct GpTimerConfig lowPriTimerConfig = {
    .frequency = 1000,
    .priority = 1,
    .channel = 0
};

static const struct GpTimerConfig highPriTimerConfig = {
    .frequency = 1000,
    .priority = 3,
    .channel = 1
};
/*----------------------------------------------------------------------------*/
static void lowPriCallback(void *argument)
{
  struct Pin * const pin = argument;

  pinToggle(*pin);
  mdelay(1000);
  pinToggle(*pin);
}
/*----------------------------------------------------------------------------*/
static void highPriCallback(void *argument)
{
  struct Pin * const pin = argument;

  pinToggle(*pin);
  mdelay(100);
  pinToggle(*pin);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  static const bool ENABLE_GROUPING = true;

  boardSetupClockExt();

  struct Pin led[] = {
      pinInit(BOARD_LED_0),
      pinInit(BOARD_LED_1)
  };

  pinOutput(led[0], BOARD_LED_INV);
  pinOutput(led[1], BOARD_LED_INV);

  const uint8_t groups = nvicGetPriorityGrouping();

  if (ENABLE_GROUPING)
  {
    /* Enabling of the led[0] will stop flashing of the led[1] */
    nvicSetPriorityGrouping(groups + 2);
  }
  else
  {
    /* LED's will flash independently */
    nvicSetPriorityGrouping(groups + 1);
  }

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
