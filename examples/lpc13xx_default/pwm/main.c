/*
 * main.c
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/pin.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/gptimer_pwm.h>
#include <halm/platform/nxp/lpc13xx/clocking.h>
/*----------------------------------------------------------------------------*/
#define SINGLE_EDGE_PWM_A_PIN PIN(1, 1)
#define SINGLE_EDGE_PWM_B_PIN PIN(1, 2)
/*----------------------------------------------------------------------------*/
struct Pwm *singleEdgeA;
struct Pwm *singleEdgeB;
/*----------------------------------------------------------------------------*/
static const struct GpTimerPwmUnitConfig pwmUnitConfig = {
    .frequency = 1000,
    .resolution = 10,
    .channel = GPTIMER_CT32B1
};

static const struct GpTimerConfig timerConfig = {
    .frequency = 1000,
    .channel = GPTIMER_CT32B0
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
  while (!clockReady(MainClock));
}
/*----------------------------------------------------------------------------*/
static void onTimerOverlow(void *arg __attribute__((unused)))
{
  static uint32_t value = 0;
  const uint32_t resolution = pwmGetResolution(singleEdgeA);

  pwmSetDuration(singleEdgeB, value);

  value = value < resolution ? value + 1 : 0;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

  struct GpTimerPwmUnit * const pwmUnit = init(GpTimerPwmUnit, &pwmUnitConfig);
  assert(pwmUnit);

  singleEdgeA = gpTimerPwmCreate(pwmUnit, SINGLE_EDGE_PWM_A_PIN);
  assert(singleEdgeA);
  pwmSetDuration(singleEdgeA, pwmGetResolution(singleEdgeA) / 2);
  pwmSetEnabled(singleEdgeA, true);

  singleEdgeB = gpTimerPwmCreate(pwmUnit, SINGLE_EDGE_PWM_B_PIN);
  assert(singleEdgeB);
  pwmSetEnabled(singleEdgeB, true);

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  timerSetOverflow(timer, 10);
  timerCallback(timer, onTimerOverlow, 0);
  timerSetEnabled(timer, true);

  while (1);

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
