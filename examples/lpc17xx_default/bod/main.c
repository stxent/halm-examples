/*
 * lpc17xx_default/bod/main.c
 * Copyright (C) 2018 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/nxp/bod.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN_1 PIN(1, 8)
#define LED_PIN_2 PIN(1, 9)
/*----------------------------------------------------------------------------*/
static const struct BodConfig bodConfig = {
    .eventLevel = BOD_EVENT_2V2,
    .resetLevel = BOD_RESET_DISABLED
};
/*----------------------------------------------------------------------------*/
static void onBodEvent(void *argument)
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

  struct Interrupt * const bod = init(Bod, &bodConfig);
  assert(bod);
  interruptSetCallback(bod, onBodEvent, leds);
  interruptEnable(bod);

  while (1);
  return 0;
}
