/*
 * lpc17xx_default/dac/main.c
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/lpc/dac.h>
#include <halm/platform/lpc/gptimer.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define DAC_PIN PIN(0, 26)
#define LED_PIN PIN(1, 8)

#define VOLTAGE_RANGE 65536
#define VOLTAGE_STEP  4096
/*----------------------------------------------------------------------------*/
static const struct GpTimerConfig timerConfig = {
    .frequency = 1000,
    .channel = 0
};

static const struct DacConfig dacConfig = {
    .pin = DAC_PIN,
    .value = VOLTAGE_RANGE / 2
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

  struct Interface * const dac = init(Dac, &dacConfig);
  assert(dac);

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  timerSetOverflow(timer, 1000);

  bool event = false;
  timerSetCallback(timer, onTimerOverflow, &event);
  timerEnable(timer);

  unsigned int step = 0;

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    pinSet(led);
    const uint16_t voltage =
        step * (VOLTAGE_RANGE - 1) / (VOLTAGE_RANGE / VOLTAGE_STEP);
    step = step < 16 ? step + 1 : 0;
    ifWrite(dac, &voltage, sizeof(voltage));
    pinReset(led);
  }

  return 0;
}
