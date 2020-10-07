/*
 * lpc13uxx_default/blinking_led/main.c
 * Copyright (C) 2020 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/nxp/gptimer.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN PIN(1, 13)
/*----------------------------------------------------------------------------*/
static const struct GpTimerConfig timerConfig = {
    .frequency = 1000,
    .channel = GPTIMER_CT32B0
};
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

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  timerSetOverflow(timer, 500);

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
