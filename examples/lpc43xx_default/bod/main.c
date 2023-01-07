/*
 * lpc43xx_default/bod/main.c
 * Copyright (C) 2022 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/lpc/bod.h>
#include <halm/platform/lpc/clocking.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN_1 PIN(PORT_7, 7)
#define LED_PIN_2 PIN(PORT_C, 11)
/*----------------------------------------------------------------------------*/
static const struct BodConfig bodConfig = {
    .eventLevel = BOD_EVENT_3V05,
    .resetLevel = BOD_RESET_2V1
};

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_INTERNAL
};
/*----------------------------------------------------------------------------*/
static void onPowerEvent(void *argument)
{
  struct Pin * const leds = argument;

  pinSet(leds[0]);
  pinToggle(leds[1]);
}
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(MainClock, &mainClockConfig);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

  struct Pin leds[2] = {
      pinInit(LED_PIN_1),
      pinInit(LED_PIN_2)
  };
  pinOutput(leds[0], false);
  pinOutput(leds[1], false);

  struct Interrupt * const bod = init(Bod, &bodConfig);
  assert(bod);
  interruptSetCallback(bod, onPowerEvent, leds);
  interruptEnable(bod);

  while (1);
  return 0;
}
