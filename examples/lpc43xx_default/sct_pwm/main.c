/*
 * lpc43xx_default/sct_pwm/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/pin.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/sct_pwm.h>
#include <halm/platform/nxp/lpc43xx/clocking.h>
/*----------------------------------------------------------------------------*/
#define DOUBLE_EDGE_PWM_A_PIN PIN(7, 0)
#define DOUBLE_EDGE_PWM_B_PIN PIN(7, 1)
#define SINGLE_EDGE_PWM_A_PIN PIN(7, 4)
#define SINGLE_EDGE_PWM_B_PIN PIN(7, 5)

#define TEST_UNIFIED
/*----------------------------------------------------------------------------*/
static struct Pwm *doubleEdgeA;
static struct Pwm *doubleEdgeB;
static struct Pwm *singleEdgeA;
static struct Pwm *singleEdgeB;
/*----------------------------------------------------------------------------*/
#ifdef TEST_UNIFIED
static const struct SctPwmUnitConfig pwmUnitConfig = {
    .frequency = 10000,
    .resolution = 10,
    .part = SCT_UNIFIED,
    .channel = 0
};
#else
static const struct SctPwmUnitConfig pwmUnitConfigs[] = {
    {
        .frequency = 10000,
        .resolution = 10,
        .part = SCT_LOW,
        .channel = 0
    },
    {
        .frequency = 10000,
        .resolution = 10,
        .part = SCT_HIGH,
        .channel = 0
    }
};
#endif

static const struct GpTimerConfig timerConfig = {
    .frequency = 10000,
    .channel = 0
};
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct CommonClockConfig mainClkConfig = {
    .source = CLOCK_EXTERNAL
};

static const struct CommonClockConfig initialClkConfig = {
    .source = CLOCK_INTERNAL
};
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(MainClock, &initialClkConfig);

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainClock, &mainClkConfig);
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

#ifdef TEST_UNIFIED
  struct GpPwmUnit * const pwmUnitA = init(SctPwmUnit, &pwmUnitConfig);
  assert(pwmUnitA);
  struct GpPwmUnit * const pwmUnitB = pwmUnitA;
#else
  struct GpPwmUnit * const pwmUnitA = init(SctPwmUnit, &pwmUnitConfigs[0]);
  assert(pwmUnitA);
  struct GpPwmUnit * const pwmUnitB = init(SctPwmUnit, &pwmUnitConfigs[1]);
  assert(pwmUnitB);
#endif

  singleEdgeA = sctPwmCreate(pwmUnitA, SINGLE_EDGE_PWM_A_PIN);
  assert(singleEdgeA);
  pwmSetDuration(singleEdgeA, pwmGetResolution(singleEdgeA) / 2);
  pwmEnable(singleEdgeA);

  singleEdgeB = sctPwmCreate(pwmUnitB, SINGLE_EDGE_PWM_B_PIN);
  assert(singleEdgeB);
  pwmEnable(singleEdgeB);

  doubleEdgeA = sctPwmCreateDoubleEdge(pwmUnitA, DOUBLE_EDGE_PWM_A_PIN);
  assert(doubleEdgeA);
  pwmEnable(doubleEdgeA);

  doubleEdgeB = sctPwmCreateDoubleEdge(pwmUnitB, DOUBLE_EDGE_PWM_B_PIN);
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
/*----------------------------------------------------------------------------*/
void __assert_func(const char *file __attribute__((unused)),
    int line __attribute__((unused)),
    const char *func __attribute__((unused)),
    const char *expr __attribute__((unused)))
{
  while (1);
}
