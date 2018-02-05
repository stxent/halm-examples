/*
 * lpc17xx_default/pwm/main.c
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
  pwmEnable(singleEdgeA);

  singleEdgeB = gpPwmCreate(pwmUnit, SINGLE_EDGE_PWM_B_PIN);
  assert(singleEdgeB);
  pwmEnable(singleEdgeB);

  doubleEdgeA = gpPwmCreateDoubleEdge(pwmUnit, DOUBLE_EDGE_PWM_A_PIN);
  assert(doubleEdgeA);
  pwmEnable(doubleEdgeA);

  doubleEdgeB = gpPwmCreateDoubleEdge(pwmUnit, DOUBLE_EDGE_PWM_B_PIN);
  assert(doubleEdgeB);
  pwmEnable(doubleEdgeB);

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  timerSetOverflow(timer, 10);
  timerSetCallback(timer, onTimerOverflow, 0);
  timerEnable(timer);

  while (1);
  return 0;
}
