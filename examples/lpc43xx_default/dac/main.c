/*
 * main.c
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/pin.h>
#include <halm/platform/nxp/dac.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/lpc43xx/clocking.h>
/*----------------------------------------------------------------------------*/
#define DAC_PIN PIN(PORT_4, 4)
#define LED_PIN PIN(PORT_6, 6)

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

static const struct CommonClockConfig mainClkConfig = {
    .source = CLOCK_INTERNAL
};
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(MainClock, &mainClkConfig);
}
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  struct Interface * const dac = init(Dac, &dacConfig);
  assert(dac);

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  timerSetOverflow(timer, 1000);

  bool event = false;
  timerCallback(timer, onTimerOverflow, &event);
  timerSetEnabled(timer, true);

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
