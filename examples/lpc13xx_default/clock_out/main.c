/*
 * main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/pin.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/gptimer_counter.h>
#include <halm/platform/nxp/lpc13xx/clocking.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN     PIN(3, 0)

#define EVENT_PIN   PIN(1, 0)
#define OUTPUT_PIN  PIN(0, 1)
/*----------------------------------------------------------------------------*/
static const struct GpTimerConfig timerConfig = {
    .frequency = 1000,
    .channel = GPTIMER_CT16B0
};

static const struct GpTimerCounterConfig counterConfig = {
    .edge = PIN_RISING,
    .pin = EVENT_PIN,
    .channel = GPTIMER_CT32B1
};
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct CommonClockConfig mainClkConfig = {
    .source = CLOCK_EXTERNAL
};

static const struct ClockOutputConfig outputClkConfig = {
    .source = CLOCK_MAIN,
    .divisor = 200,
    .pin = OUTPUT_PIN
};
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainClock, &mainClkConfig);

  clockEnable(ClockOutput, &outputClkConfig);
  while (!clockReady(ClockOutput));
}
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  const uint32_t frequency = extOscConfig.frequency / outputClkConfig.divisor;
  const uint32_t maxPeriod = frequency + frequency / 1000;
  const uint32_t minPeriod = frequency - frequency / 1000;

  setupClock();

  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, true);

  struct Timer * const counter = init(GpTimerCounter, &counterConfig);
  assert(counter);

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  timerSetOverflow(timer, 1000);

  bool event = false;
  timerSetCallback(timer, onTimerOverflow, &event);

  timerEnable(counter);
  timerEnable(timer);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    const uint32_t period = timerGetValue(counter);
    timerSetValue(counter, 0);

    if (period >= minPeriod && period <= maxPeriod)
      pinReset(led);
    else
      pinSet(led);
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
