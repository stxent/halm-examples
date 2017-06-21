/*
 * lpc17xx_default/adc_burst/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/pin.h>
#include <halm/platform/nxp/adc.h>
#include <halm/platform/nxp/adc_burst.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/lpc17xx/clocking.h>
/*----------------------------------------------------------------------------*/
#define INPUT_PIN PIN(0, 25)
#define LED_PIN   PIN(1, 8)

#define TEST_ZEROCOPY
/*----------------------------------------------------------------------------*/
static const struct AdcUnitConfig adcUnitConfig = {
    .priority = 2,
    .channel = 0
};

static const struct GpTimerConfig eventTimerConfig = {
    .frequency = 1000000,
    .event = GPTIMER_MATCH1,
    .channel = 1
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
static void onEvent(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

  struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  struct AdcUnit * const adcUnit = init(AdcUnit, &adcUnitConfig);
  assert(adcUnit);

  const struct AdcBurstConfig adcConfig = {
      .parent = adcUnit,
      .event = ADC_TIMER1_MAT1,
      .pin = INPUT_PIN
  };
  struct Interface * const adc = init(AdcBurst, &adcConfig);
  assert(adc);

  struct Timer * const eventTimer = init(GpTimer, &eventTimerConfig);
  assert(eventTimer);
  timerSetOverflow(eventTimer, 500);

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  timerSetOverflow(timer, 1000);

  uint16_t samples[32];
  bool event = false;

#ifdef TEST_ZEROCOPY
  bool completed;
  enum Result res;

  res = ifSetParam(adc, IF_ZEROCOPY, 0);
  assert(res == E_OK);
  res = ifSetCallback(adc, onEvent, &completed);
  assert(res == E_OK);

  (void)res; /* Suppress warning */
#endif

  timerSetCallback(timer, onEvent, &event);
  timerEnable(timer);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    pinSet(led);
    timerEnable(eventTimer);

    const size_t bytesRead = ifRead(adc, samples, sizeof(samples));
    assert(bytesRead == sizeof(samples));
    (void)bytesRead; /* Suppress warning */

#ifdef TEST_ZEROCOPY
    while (!completed)
      barrier();
    completed = false;
#endif

    timerDisable(eventTimer);
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
