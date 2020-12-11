/*
 * lpc13xx_default/pwm_tuner/main.c
 * Copyright (C) 2018 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/lpc/adc_bus.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/gptimer_pwm.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define SINGLE_EDGE_PIN     PIN(1, 2)
#define SINGLE_EDGE_REF_PIN PIN(1, 1)
/*----------------------------------------------------------------------------*/
static struct Pwm *singleEdge;
static struct Pwm *singleEdgeRef;
static struct Interface *adc;
/*----------------------------------------------------------------------------*/
static const struct GpTimerPwmUnitConfig pwmUnitConfig = {
    .frequency = 1000,
    .resolution = 10,
    .channel = GPTIMER_CT32B1
};

/* Should be sorted by ADC channel number */
static const PinNumber adcBusPins[] = {
    PIN(1, 0), 0
};

static const struct AdcBusConfig adcBusConfig = {
    .pins = adcBusPins,
    .frequency = 1100,
    .event = ADC_BURST,
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
static void onConversionCompleted(void *arg __attribute__((unused)))
{
  static uint16_t duration = 0;
  static uint16_t voltage = 0;

  const uint32_t resolution = pwmGetResolution(singleEdge);
  const uint32_t div = (65535 + resolution) / (resolution + 1);
  const uint16_t steps = voltage / div;

  /* Start conversion and process a result in the next invocation of callback */
  ifRead(adc, &voltage, sizeof(voltage));

  if (steps != duration)
  {
    duration = steps;
    pwmSetDuration(singleEdge, duration);
  }
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

  struct GpTimerPwmUnit * const pwmUnit = init(GpTimerPwmUnit, &pwmUnitConfig);
  assert(pwmUnit);

  singleEdgeRef = gpTimerPwmCreate(pwmUnit, SINGLE_EDGE_PIN);
  assert(singleEdgeRef);
  pwmSetEdges(singleEdgeRef, 0, pwmGetResolution(singleEdgeRef) / 2);
  pwmEnable(singleEdgeRef);

  singleEdge = gpTimerPwmCreate(pwmUnit, SINGLE_EDGE_REF_PIN);
  assert(singleEdge);
  pwmSetEdges(singleEdge, 0, 0);
  pwmEnable(singleEdge);

  adc = init(AdcBus, &adcBusConfig);
  assert(adc);
  ifSetParam(adc, IF_ZEROCOPY, 0);
  ifSetCallback(adc, onConversionCompleted, 0);

  /* Start callback chain */
  onConversionCompleted(0);

  while (1);
  return 0;
}
