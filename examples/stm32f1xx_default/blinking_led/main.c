/*
 * stm32f1xx_default/blinking_led/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/core/cortex/systick.h>
#include <halm/pin.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN PIN(PORT_C, 14)
/*----------------------------------------------------------------------------*/
static const struct SysTickTimerConfig timerConfig = {
    .frequency = 1000
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

  struct Timer * const timer = init(SysTickTimer, &timerConfig);
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
/*----------------------------------------------------------------------------*/
void __assert_func(const char *file __attribute__((unused)),
    int line __attribute__((unused)),
    const char *func __attribute__((unused)),
    const char *expr __attribute__((unused)))
{
  while (1);
}
