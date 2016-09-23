/*
 * main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <stdbool.h>

#include <halm/pin.h>
#include <halm/platform/nxp/gptimer.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN PIN(0, 22)
/*----------------------------------------------------------------------------*/
static struct GpTimerConfig timerConfig = {
    .frequency = 1000,
    .channel = 0
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

  timer = init(GpTimer, &timerConfig);
  assert(timer);

  timerSetOverflow(timer, 500);
  timerCallback(timer, onTimerOverflow, &event);
  timerSetEnabled(timer, true);

  uint8_t ledValue = 0;

  while (1)
  {
    while (!event)
      barrier();

    event = false;

    pinWrite(led, ledValue);
    ledValue ^= 0x01;
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
