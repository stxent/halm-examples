/*
 * lpc17xx_default/pwm_tuner/main.c
 * Copyright (C) 2018 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/pin.h>
#include <halm/platform/nxp/adc_bus.h>
#include <halm/platform/nxp/gppwm.h>
#include <halm/platform/nxp/lpc17xx/clocking.h>
/*----------------------------------------------------------------------------*/
#define DOUBLE_EDGE_PIN     PIN(1, 24)
#define SINGLE_EDGE_PIN     PIN(1, 20)
#define SINGLE_EDGE_REF_PIN PIN(1, 18)
/*----------------------------------------------------------------------------*/
static struct Pwm *doubleEdge;
static struct Pwm *singleEdge;
static struct Pwm *singleEdgeRef;
static struct Interface *adc;
/*----------------------------------------------------------------------------*/
static const struct GpPwmUnitConfig pwmUnitConfig = {
    .frequency = 1000,
    .resolution = 10,
    .channel = 0
};

/* Should be sorted by ADC channel number */
static const PinNumber adcBusPins[] = {
    PIN(0, 3), PIN(0, 2), 0
};

static const struct AdcBusConfig adcBusConfig = {
    .pins = adcBusPins,
    .frequency = 13000,
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
  static uint16_t offset = 0;
  static uint16_t voltages[2] = {0};

  const uint32_t resolution = pwmGetResolution(singleEdgeRef);
  const uint32_t div = (65535 + resolution) / (resolution + 1);
  const uint16_t steps[] = {voltages[0] / div, voltages[1] / div};

  /* Start conversion and process a result in the next invocation of callback */
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

  struct GpPwmUnit * const pwmUnit = init(GpPwmUnit, &pwmUnitConfig);
  assert(pwmUnit);

  singleEdgeRef = gpPwmCreate(pwmUnit, SINGLE_EDGE_REF_PIN);
  assert(singleEdgeRef);
  pwmSetEdges(singleEdgeRef, 0, pwmGetResolution(singleEdgeRef) / 2);
  pwmEnable(singleEdgeRef);

  singleEdge = gpPwmCreate(pwmUnit, SINGLE_EDGE_PIN);
  assert(singleEdge);
  pwmSetEdges(singleEdge, 0, 0);
  pwmEnable(singleEdge);

  doubleEdge = gpPwmCreateDoubleEdge(pwmUnit, DOUBLE_EDGE_PIN);
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
