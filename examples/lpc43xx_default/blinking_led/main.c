/*
 * main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <stdbool.h>

#include <memory.h>
#include <pin.h>
#include <platform/nxp/gptimer.h>
#include <platform/nxp/lpc43xx/clocking.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN PIN(6, 6)
/*----------------------------------------------------------------------------*/
static struct Timer *timer;
/*----------------------------------------------------------------------------*/
static const struct GpTimerConfig timerConfig = {
    .frequency = 1000,
    .channel = GPTIMER_CT32B0
};

static const struct CommonClockConfig mainClkConfig = {
    .source = CLOCK_INTERNAL
};
/*----------------------------------------------------------------------------*/
static void enableClock(void)
{
  clockEnable(MainClock, &mainClkConfig);
}
/*----------------------------------------------------------------------------*/
static void ledToggle(void *argument)
{
  bool * const eventPointer = argument;

  *eventPointer = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  bool event = false;
  uint8_t ledValue = 0;
  struct Pin led;

  led = pinInit(LED_PIN);
  pinOutput(led, 0);

  enableClock();

  timer = init(GpTimer, &timerConfig);
  assert(timer);

  timerSetOverflow(timer, 500);
  timerCallback(timer, ledToggle, &event);
  timerSetEnabled(timer, true);

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
