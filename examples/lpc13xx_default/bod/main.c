/*
 * lpc13xx_default/bod/main.c
 * Copyright (C) 2022 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/lpc/bod.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN_1 PIN(2, 3)
#define LED_PIN_2 PIN(3, 1)
/*----------------------------------------------------------------------------*/
static const struct BodConfig bodConfig = {
    .eventLevel = BOD_EVENT_2V87,
    .resetLevel = BOD_RESET_DISABLED
};
/*----------------------------------------------------------------------------*/
static void onPowerEvent(void *argument)
{
  struct Pin * const leds = argument;

  pinSet(leds[0]);
  pinToggle(leds[1]);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
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
