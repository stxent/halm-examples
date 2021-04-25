/*
 * lpc43xx_default/rit/main.c
 * Copyright (C) 2021 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/rit.h>
#include <halm/pin.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN PIN(6, 6)
/*----------------------------------------------------------------------------*/
static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_INTERNAL
};
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(MainClock, &mainClockConfig);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

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
