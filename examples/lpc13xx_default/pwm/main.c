/*
 * lpc13xx_default/pwm/main.c
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
static struct Pwm *singleEdgeA;
static struct Pwm *singleEdgeB;
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
static void onTimerOverflow(void *arg __attribute__((unused)))
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
  pwmEnable(singleEdgeA);

  singleEdgeB = gpTimerPwmCreate(pwmUnit, SINGLE_EDGE_PWM_B_PIN);
  assert(singleEdgeB);
  pwmEnable(singleEdgeB);

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  timerSetOverflow(timer, 10);
  timerSetCallback(timer, onTimerOverflow, 0);
  timerEnable(timer);

  while (1);
  return 0;
}
