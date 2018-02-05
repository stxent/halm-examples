/*
 * lpc43xx_default/adc_dma/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/pin.h>
#include <halm/platform/nxp/adc_dma.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/lpc43xx/clocking.h>
/*----------------------------------------------------------------------------*/
#define ADC_RATE      250000
#define BUFFER_COUNT  2
#define BUFFER_SIZE   4096
#define LED_PIN       PIN(PORT_6, 6)
/*----------------------------------------------------------------------------*/
static const struct AdcUnitConfig adcUnitConfig = {
    .frequency = 4000000,
    .channel = 0
};

static const struct GpTimerConfig timerConfig = {
    .frequency = 4000000,
    .event = GPTIMER_MATCH3,
    .channel = 3
};
/*----------------------------------------------------------------------------*/
static const struct GenericClockConfig initialClockConfig = {
    .source = CLOCK_INTERNAL
};

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_PLL
};

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000,
    .bypass = false
};

static const struct PllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 1,
    .multiplier = 17
};
/*----------------------------------------------------------------------------*/
static uint16_t buffers[BUFFER_COUNT][BUFFER_SIZE];
static bool event = true;
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(MainClock, &initialClockConfig);

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &mainClockConfig);

  clockEnable(Apb3Clock, &mainClockConfig);
  while (!clockReady(Apb3Clock));
}
/*----------------------------------------------------------------------------*/
static void onConversionCompleted(void *argument)
{
  struct Interface * const adc = argument;
  size_t count;

  if (ifGetParam(adc, IF_AVAILABLE, &count) == E_OK && count > 0)
    event = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  struct AdcUnit * const adcUnit = init(AdcUnit, &adcUnitConfig);
  assert(adcUnit);

  const struct AdcDmaConfig adcConfig = {
      .parent = adcUnit,
      .event = ADC_CTOUT_15,
      .pin = PIN(PORT_ADC, 0),
      .dma = 0
  };

  struct Interface * const adc = init(AdcDma, &adcConfig);
  assert(adc);
  ifSetCallback(adc, onConversionCompleted, adc);

  struct Timer * const conversionTimer = init(GpTimer, &timerConfig);
  assert(conversionTimer);
  /*
   * The overflow frequency of the timer should be two times higher
   * than that of the hardware events for ADC.
   */
  timerSetOverflow(conversionTimer, timerConfig.frequency / (ADC_RATE * 2));

  unsigned int iteration = 0;
  size_t count;

  /* Enqueue buffers */
  while (ifGetParam(adc, IF_AVAILABLE, &count) == E_OK && count > 0)
  {
    const size_t index = iteration++ % BUFFER_COUNT;
    ifRead(adc, buffers[index], sizeof(buffers[index]));
  }

  /* Start conversion */
  timerEnable(conversionTimer);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    pinSet(led);
    while (ifGetParam(adc, IF_AVAILABLE, &count) == E_OK && count > 0)
    {
      const size_t index = iteration++ % BUFFER_COUNT;
      ifRead(adc, buffers[index], sizeof(buffers[index]));
    }
    pinReset(led);
  }

  return 0;
}
