/*
 * lpc43xx_default/wdt/main.c
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/gptimer.h>
#include <halm/platform/lpc/wdt.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define INPUT_PIN     PIN(3, 1)
#define FAIL_LED_PIN  PIN(6, 6)
#define WORK_LED_PIN  PIN(6, 7)
/*----------------------------------------------------------------------------*/
static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_INTERNAL
};

static struct GpTimerConfig timerConfig = {
    .frequency = 1000,
    .channel = 0
};

static const struct WdtConfig wdtConfig = {
    .period = 1000
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

  const struct Pin input = pinInit(INPUT_PIN);
  pinInput(input);
  pinSetPull(input, PIN_PULLDOWN);

  const struct Pin failLed = pinInit(FAIL_LED_PIN);
  pinOutput(failLed, true);

  const struct Pin workLed = pinInit(WORK_LED_PIN);
  pinOutput(workLed, true);

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  timerSetOverflow(timer, 100);

  struct Watchdog * const wdt = init(Wdt, &wdtConfig);
  assert(wdt);

  bool event = false;
  timerSetCallback(timer, onTimerOverflow, &event);
  timerEnable(timer);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    if (pinRead(input))
    {
      watchdogReload(wdt);
      pinReset(failLed);
      pinSet(workLed);
    }
    else
    {
      pinSet(failLed);
      pinReset(workLed);
    }
  }

  return 0;
}
