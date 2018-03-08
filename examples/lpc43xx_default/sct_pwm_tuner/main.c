/*
 * lpc43xx_default/pwm_tuner/main.c
 * Copyright (C) 2018 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/pin.h>
#include <halm/platform/nxp/adc_bus.h>
#include <halm/platform/nxp/sct_pwm.h>
#include <halm/platform/nxp/lpc43xx/clocking.h>
/*----------------------------------------------------------------------------*/
#define DOUBLE_EDGE_PIN     PIN(7, 5)
#define SINGLE_EDGE_PIN     PIN(7, 4)
#define SINGLE_EDGE_REF_PIN PIN(7, 0)

#define TEST_UNIFIED
/*----------------------------------------------------------------------------*/
static struct Pwm *doubleEdge;
static struct Pwm *singleEdge;
static struct Pwm *singleEdgeRef;
static struct Interface *adc;
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

/* Should be sorted by ADC channel number */
static const PinNumber adcBusPins[] = {
    PIN(PORT_ADC, 0), PIN(PORT_ADC, 1), 0
};

static const struct AdcBusConfig adcBusConfig = {
    .pins = adcBusPins,
    .frequency = 2200,
    .event = ADC_BURST,
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
static void onConversionCompleted(void *arg __attribute__((unused)))
{
  static uint16_t duration = 0;
  static uint16_t offset = 0;
  static uint16_t voltages[2] = {0};

  const uint32_t resolution = pwmGetResolution(singleEdge);
  const uint32_t div = (65535 + resolution) / (resolution + 1);
  const uint16_t steps[] = {voltages[0] / div, voltages[1] / div};

  /* Request conversion of two channels in a non-blocking way */
  ifRead(adc, voltages, sizeof(voltages));

  if (steps[0] != duration)
  {
    duration = steps[0];
    pwmSetDuration(doubleEdge, duration);
    pwmSetDuration(singleEdge, duration);
  }

  if (steps[1] != offset)
  {
    offset = steps[1];

    const uint32_t leading = offset % resolution;
    const uint32_t trailing = duration < resolution ?
        ((offset + duration) % resolution) : resolution;

    pwmSetEdges(doubleEdge, leading, trailing);
  }
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

  doubleEdge = sctPwmCreateDoubleEdge(pwmUnitB, DOUBLE_EDGE_PIN);
  assert(doubleEdge);
  pwmSetEdges(doubleEdge, 0, 0);
  pwmEnable(doubleEdge);

  adc = init(AdcBus, &adcBusConfig);
  assert(adc);
  ifSetParam(adc, IF_ZEROCOPY, 0);
  ifSetCallback(adc, onConversionCompleted, 0);

  /* Start callback chain */
  onConversionCompleted(0);

  while (1);
  return 0;
}
