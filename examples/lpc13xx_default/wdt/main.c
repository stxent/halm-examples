/*
 * main.c
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/pin.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/lpc13xx/clocking.h>
#include <halm/platform/nxp/wdt.h>
/*----------------------------------------------------------------------------*/
#define INPUT_PIN     PIN(1, 0)
#define FAIL_LED_PIN  PIN(2, 3)
#define WORK_LED_PIN  PIN(3, 1)
/*----------------------------------------------------------------------------*/
static struct GpTimerConfig timerConfig = {
    .frequency = 1000,
    .channel = GPTIMER_CT32B0
};

static const struct WdtConfig wdtConfig = {
    .period = 1000
};
/*----------------------------------------------------------------------------*/
static const struct WdtOscConfig wdtOscConfig = {
    .frequency = WDT_FREQ_1050
};

static const struct CommonClockConfig wdtClockConfig = {
    .source = CLOCK_WDT
};
/*----------------------------------------------------------------------------*/
void setupClock()
{
  clockEnable(WdtOsc, &wdtOscConfig);
  while (!clockReady(WdtOsc));

  clockEnable(WdtClock, &wdtClockConfig);
  while (!clockReady(WdtClock));
}
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  *(bool *)argument = true;
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
  timerCallback(timer, onTimerOverflow, &event);
  timerSetEnabled(timer, true);

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
