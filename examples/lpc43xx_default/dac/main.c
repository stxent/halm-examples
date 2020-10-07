/*
 * lpc43xx_default/dac/main.c
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/nxp/dac.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/lpc43xx/clocking.h>
#include <assert.h>
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
    .value = VOLTAGE_RANGE / 2,
    .pin = DAC_PIN
};

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_INTERNAL
};
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(MainClock, &mainClockConfig);
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
