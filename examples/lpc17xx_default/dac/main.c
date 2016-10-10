/*
 * main.c
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/pin.h>
#include <halm/platform/nxp/dac.h>
#include <halm/platform/nxp/gptimer.h>
/*----------------------------------------------------------------------------*/
#define DAC_PIN PIN(0, 26)
#define LED_PIN PIN(0, 22)
/*----------------------------------------------------------------------------*/
static const struct GpTimerConfig timerConfig = {
    .frequency = 1000,
    .channel = 0
};

static const struct DacConfig dacConfig = {
    .pin = DAC_PIN,
    .value = 32768
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
  pinOutput(led, 0);

  struct Interface * const dac = init(Dac, &dacConfig);
  assert(dac);

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  timerSetOverflow(timer, 1000);

  bool event = false;
  timerCallback(timer, onTimerOverflow, &event);
  timerSetEnabled(timer, true);

  uint16_t voltage = dacConfig.value;

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    pinSet(led);
    voltage += 4096;
    ifWrite(dac, &voltage, sizeof(voltage));
    pinReset(led);
  }

  return 0;
}
