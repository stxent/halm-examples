/*
 * stm32f0xx_default/blinking_led/main.c
 * Copyright (C) 2020 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/core/cortex/systick.h>
#include <halm/pin.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN PIN(PORT_C, 9)
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

  /*
   * Core frequency and SysTick resolution must be kept in mind
   * when configuring timer overflow.
   */
  struct Timer * const timer = init(SysTickTimer, 0);
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
