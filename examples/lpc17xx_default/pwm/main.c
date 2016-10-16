/*
 * main.c
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/pin.h>
#include <halm/platform/nxp/gppwm.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/lpc17xx/clocking.h>
/*----------------------------------------------------------------------------*/
#define DOUBLE_EDGE_PWM_A_PIN PIN(2, 3)
#define DOUBLE_EDGE_PWM_B_PIN PIN(2, 5)
#define SINGLE_EDGE_PWM_A_PIN PIN(2, 0)
#define SINGLE_EDGE_PWM_B_PIN PIN(2, 1)
/*----------------------------------------------------------------------------*/
static struct Pwm *doubleEdgeA;
static struct Pwm *doubleEdgeB;
static struct Pwm *singleEdgeA;
static struct Pwm *singleEdgeB;
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
static void onTimerOverlow(void *arg __attribute__((unused)))
{
  static uint32_t value = 0;
  const uint32_t resolution = pwmGetResolution(singleEdgeA);

  pwmSetDuration(singleEdgeB, value);
  pwmSetDuration(doubleEdgeA, value);
  pwmSetEdges(doubleEdgeB, value < resolution ? value : 0,
      (value + resolution / 2) % resolution);

  value = value < resolution ? value + 1 : 0;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

  struct GpPwmUnit * const pwmUnit = init(GpPwmUnit, &pwmUnitConfig);
  assert(pwmUnit);

  singleEdgeA = gpPwmCreate(pwmUnit, SINGLE_EDGE_PWM_A_PIN);
  assert(singleEdgeA);
  pwmSetDuration(singleEdgeA, pwmGetResolution(singleEdgeA) / 2);
  pwmSetEnabled(singleEdgeA, true);

  singleEdgeB = gpPwmCreate(pwmUnit, SINGLE_EDGE_PWM_B_PIN);
  assert(singleEdgeB);
  pwmSetEnabled(singleEdgeB, true);

  doubleEdgeA = gpPwmCreateDoubleEdge(pwmUnit, DOUBLE_EDGE_PWM_A_PIN);
  assert(doubleEdgeA);
  pwmSetEnabled(doubleEdgeA, true);

  doubleEdgeB = gpPwmCreateDoubleEdge(pwmUnit, DOUBLE_EDGE_PWM_B_PIN);
  assert(doubleEdgeB);
  pwmSetEnabled(doubleEdgeB, true);

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
