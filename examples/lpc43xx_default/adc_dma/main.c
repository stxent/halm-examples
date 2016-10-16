/*
 * main.c
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
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000,
    .bypass = false
};

static const struct PllConfig sysPllConfig = {
    .multiplier = 17,
    .divisor = 1,
    .source = CLOCK_EXTERNAL
};

static const struct CommonClockConfig mainClkConfig = {
    .source = CLOCK_PLL
};

static const struct CommonClockConfig initialClock = {
    .source = CLOCK_INTERNAL
};
/*----------------------------------------------------------------------------*/
static uint16_t buffers[BUFFER_COUNT][BUFFER_SIZE];
static bool event = true;
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(MainClock, &initialClock);

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &mainClkConfig);

  clockEnable(Apb3Clock, &mainClkConfig);
  while (!clockReady(Apb3Clock));
}
/*----------------------------------------------------------------------------*/
static void onConversionCompleted(void *argument)
{
  struct Interface * const adc = argument;
  size_t count;

  if (ifGet(adc, IF_AVAILABLE, &count) == E_OK && count > 0)
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
  ifCallback(adc, onConversionCompleted, adc);

  struct Timer * const conversionTimer = init(GpTimer, &timerConfig);
  assert(conversionTimer);
  /*
   * GpTimer uses match output toggle so the overflow rate for timer
   * should be two times higher than frequency of the hardware events.
   */
  timerSetOverflow(conversionTimer, timerConfig.frequency / (ADC_RATE * 2));

  unsigned int iteration = 0;
  size_t count;

  /* Enqueue buffers */
  while (ifGet(adc, IF_AVAILABLE, &count) == E_OK && count > 0)
  {
    const size_t index = iteration++ % BUFFER_COUNT;
    ifRead(adc, buffers[index], sizeof(buffers[index]));
  }

  /* Start conversion */
  timerSetEnabled(conversionTimer, true);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    pinSet(led);
    while (ifGet(adc, IF_AVAILABLE, &count) == E_OK && count > 0)
    {
      const size_t index = iteration++ % BUFFER_COUNT;
      ifRead(adc, buffers[index], sizeof(buffers[index]));
    }
    pinReset(led);
  }

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
