/*
 * lpc17xx_default/pm_sleep/main.c
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/delay.h>
#include <halm/gpio_bus.h>
#include <halm/pm.h>
#include <halm/timer.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
static const struct SimpleGpioBusConfig busConfig = {
    .pins = (const PinNumber []){
        BOARD_LED_0, BOARD_LED_1, BOARD_LED_2, 0
    },
    .initial = 0,
    .direction = PIN_OUTPUT
};
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument __attribute__((unused)))
{
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  boardSetupClockExt();

  struct GpioBus * const leds = init(SimpleGpioBus, &busConfig);
  assert(leds);

  struct Timer * const timer = boardSetupTimer();
  timerSetOverflow(timer, timerGetFrequency(timer) * 5);
  timerSetCallback(timer, onTimerOverflow, 0);
  timerEnable(timer);

  uint32_t state = 1;

  while (1)
  {
    for (unsigned int i = 0; i < 5; ++i)
    {
      gpioBusWrite(leds, i % 2 ? 0 : state);
      mdelay(500);
    }

    gpioBusWrite(leds, 0);
    state = state < 7 ? state + 1 : 1;

    pmChangeState(PM_SLEEP);
  }

  return 0;
}
