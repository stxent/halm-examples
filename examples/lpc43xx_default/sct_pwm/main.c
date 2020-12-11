/*
 * lpc43xx_default/sct_pwm/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/gptimer.h>
#include <halm/platform/lpc/sct_pwm.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define DOUBLE_EDGE_PIN     PIN(7, 5)
#define SINGLE_EDGE_PIN     PIN(7, 4)
#define SINGLE_EDGE_REF_PIN PIN(7, 0)

#define TEST_UNIFIED
/*----------------------------------------------------------------------------*/
static struct Pwm *doubleEdge;
static struct Pwm *singleEdge;
static struct Pwm *singleEdgeRef;
/*----------------------------------------------------------------------------*/
#ifdef TEST_UNIFIED
static const struct SctPwmUnitConfig pwmUnitConfig = {
    .frequency = 1000,
    .resolution = 10,
    .part = SCT_UNIFIED,
    .channel = 0
};
#else
static const struct SctPwmUnitConfig pwmUnitConfigs[] = {
    {
        .frequency = 1000,
        .resolution = 10,
        .part = SCT_LOW,
        .channel = 0
    },
    {
        .frequency = 1000,
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

static const struct GenericDividerConfig dividerConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 12
};

static const struct GenericClockConfig initialClockConfig = {
    .source = CLOCK_INTERNAL
};

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_IDIVB
};
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(MainClock, &initialClockConfig);

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(DividerB, &dividerConfig);
  while (!clockReady(DividerB));

  clockEnable(MainClock, &mainClockConfig);
}
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *arg __attribute__((unused)))
{
  static uint32_t iteration = 0;

  const uint32_t resolution = pwmGetResolution(singleEdgeRef);
  const uint32_t duration = (iteration / 100) % (resolution + 1);
  const uint32_t trailing = duration < resolution ?
      ((iteration + duration) % resolution) : resolution;

  pwmSetEdges(singleEdge, 0, iteration % (resolution + 1));
  pwmSetEdges(doubleEdge, iteration % resolution, trailing);
  ++iteration;
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

  singleEdgeRef = sctPwmCreate(pwmUnitA, SINGLE_EDGE_REF_PIN);
  assert(singleEdgeRef);
  pwmSetEdges(singleEdgeRef, 0, pwmGetResolution(singleEdgeRef) / 2);
  pwmEnable(singleEdgeRef);

  singleEdge = sctPwmCreate(pwmUnitB, SINGLE_EDGE_PIN);
  assert(singleEdge);
  pwmSetEdges(singleEdge, 0, 0);
  pwmEnable(singleEdge);

  doubleEdge = sctPwmCreateDoubleEdge(pwmUnitA, DOUBLE_EDGE_PIN);
  assert(doubleEdge);
  pwmSetEdges(doubleEdge, 0, 0);
  pwmEnable(doubleEdge);

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  timerSetOverflow(timer, 10);
  timerSetCallback(timer, onTimerOverflow, 0);
  timerEnable(timer);

  while (1);
  return 0;
}
