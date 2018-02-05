/*
 * lpc13xx_default/counter/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/pin.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/gptimer_counter.h>
#include <halm/platform/nxp/gptimer_pwm.h>
#include <halm/platform/nxp/lpc13xx/clocking.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN     PIN(3, 0)

#define EVENT_PIN   PIN(1, 0)
#define OUTPUT_PIN  PIN(0, 1)
/*----------------------------------------------------------------------------*/
static const struct GpTimerPwmUnitConfig pwmUnitConfig = {
    .frequency = 1000,
    .resolution = 10,
    .channel = GPTIMER_CT32B0
};

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

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_EXTERNAL
};
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainClock, &mainClockConfig);
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
  pinOutput(led, true);

  struct GpTimerPwmUnit * const pwmUnit = init(GpTimerPwmUnit, &pwmUnitConfig);
  assert(pwmUnit);
  struct Pwm * const pwmOutput = gpTimerPwmCreate(pwmUnit, OUTPUT_PIN);
  assert(pwmOutput);
  pwmSetDuration(pwmOutput, pwmGetResolution(pwmOutput) / 2);

  struct Timer * const counter = init(GpTimerCounter, &counterConfig);
  assert(counter);

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  timerSetOverflow(timer, 1000);

  bool event = false;
  timerSetCallback(timer, onTimerOverflow, &event);

  timerEnable(counter);
  pwmEnable(pwmOutput);
  timerEnable(timer);

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
