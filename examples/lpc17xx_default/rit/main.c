/*
 * lpc17xx_default/rit/main.c
 * Copyright (C) 2018 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/platform/nxp/rit.h>
#include <halm/pin.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN PIN(1, 8)
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, true);

  struct Timer * const timer = init(Rit, 0);
  assert(timer);
  timerSetOverflow(timer, timerGetFrequency(timer) / 2);

  bool event = false;
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
