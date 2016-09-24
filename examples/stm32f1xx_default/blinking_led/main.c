/*
 * main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/core/cortex/systick.h>
#include <halm/pin.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN PIN(PORT_C, 13)
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
  struct Timer *timer;
  struct Pin led;
  bool event = false;

  led = pinInit(LED_PIN);
  pinOutput(led, 0);

  timer = init(SysTickTimer, &timerConfig);
  assert(timer);

  timerSetOverflow(timer, 500);
  timerCallback(timer, onTimerOverflow, &event);
  timerSetEnabled(timer, true);

  bool ledValue = 0;

  while (1)
  {
    while (!event)
      barrier();

    event = false;

    pinWrite(led, ledValue);
    ledValue = !ledValue;
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
