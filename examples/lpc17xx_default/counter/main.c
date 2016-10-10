/*
 * main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/pin.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/gptimer_counter.h>
#include <halm/platform/nxp/gppwm.h>
#include <halm/platform/nxp/lpc17xx/clocking.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN     PIN(0, 22)

#define EVENT_PIN   PIN(1, 19)
#define OUTPUT_PIN  PIN(1, 18)
/*----------------------------------------------------------------------------*/
static const struct GpPwmUnitConfig pwmUnitConfig = {
    .frequency = 1000,
    .resolution = 10,
    .channel = 0
};

static const struct GpTimerConfig timerConfig = {
    .frequency = 1000,
    .channel = 0
};

static const struct GpTimerCounterConfig counterConfig = {
    .edge = PIN_RISING,
    .pin = EVENT_PIN,
    .channel = 1
};
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct CommonClockConfig mainClkConfig = {
    .source = CLOCK_EXTERNAL
};
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainClock, &mainClkConfig);
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

  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, 1);

  struct GpPwmUnit * const pwmUnit = init(GpPwmUnit, &pwmUnitConfig);
  assert(pwmUnit);
  struct Pwm * const pwmOutput = gpPwmCreate(pwmUnit, OUTPUT_PIN);
  assert(pwmOutput);
  pwmSetDuration(pwmOutput, pwmGetResolution(pwmOutput) / 2);

  struct Timer * const counter = init(GpTimerCounter, &counterConfig);
  assert(counter);

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  timerSetOverflow(timer, 1000);

  bool event = false;
  timerCallback(timer, onTimerOverflow, &event);

  timerSetEnabled(counter, true);
  pwmSetEnabled(pwmOutput, true);
  timerSetEnabled(timer, true);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    const uint32_t period = timerGetValue(counter);
    timerSetValue(counter, 0);

    if (period >= 999 && period <= 1001)
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
