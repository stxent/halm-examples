/*
 * lpc43xx_default/sct/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/sct_timer.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define TEST_UNIFIED
/*----------------------------------------------------------------------------*/
#ifdef TEST_UNIFIED
static const struct SctTimerConfig timerConfig = {
    .frequency = 1000000,
    .part = SCT_UNIFIED,
    .channel = 0
};
#else
static const struct SctTimerConfig timerConfigs[] = {
    {
        .frequency = 100000,
        .part = SCT_LOW,
        .channel = 0
    },
    {
        .frequency = 400000,
        .part = SCT_HIGH,
        .channel = 0
    }
};
#endif
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  pinToggle(*(struct Pin *)argument);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  boardSetupClockExt();

  struct Pin ledA = pinInit(BOARD_LED_0);
  pinOutput(ledA, BOARD_LED_INV);

  struct Pin ledB = pinInit(BOARD_LED_1);
  pinOutput(ledB, BOARD_LED_INV);

#ifdef TEST_UNIFIED
  struct Timer * const timerA = init(SctUnifiedTimer, &timerConfig);
  assert(timerA);
  timerSetOverflow(timerA, 500000);
  timerSetCallback(timerA, onTimerOverflow, &ledA);
  timerEnable(timerA);
#else
  struct Timer * const timerA = init(SctTimer, &timerConfigs[0]);
  assert(timerA);
  timerSetOverflow(timerA, 50000);
  timerSetCallback(timerA, onTimerOverflow, &ledA);
  timerEnable(timerA);

  struct Timer * const timerB = init(SctTimer, &timerConfigs[1]);
  assert(timerB);
  timerSetOverflow(timerB, 50000);
  timerSetCallback(timerB, onTimerOverflow, &ledB);
  timerEnable(timerB);
#endif

  while (1);
  return 0;
}
