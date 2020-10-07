/*
 * lpc13xx_default/adc/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/nxp/adc_oneshot.h>
#include <halm/platform/nxp/gptimer.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN   PIN(3, 0)
#define INPUT_PIN PIN(1, 11)
/*----------------------------------------------------------------------------*/
static const struct AdcOneShotConfig adcConfig = {
    .pin = INPUT_PIN,
    .channel = 0
};

static const struct GpTimerConfig timerConfig = {
    .frequency = 1000,
    .channel = GPTIMER_CT32B0
};
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  struct Interface * const adc = init(AdcOneShot, &adcConfig);
  assert(adc);

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  timerSetOverflow(timer, 1000);

  bool event = false;
  timerSetCallback(timer, onTimerOverflow, &event);
  timerEnable(timer);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    pinSet(led);

    uint16_t voltage;
    const size_t bytesRead = ifRead(adc, &voltage, sizeof(voltage));
    assert(bytesRead == sizeof(voltage));
    (void)bytesRead; /* Suppress warning */

    pinReset(led);
  }

  return 0;
}
