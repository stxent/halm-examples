/*
 * lpc11xx_default/pin_int/main.c
 * Copyright (C) 2022 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/delay.h>
#include <halm/pin.h>
#include <halm/platform/lpc/pin_int.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define EVENT_PIN PIN(1, 8)
#define LED_PIN   PIN(1, 2)
/*----------------------------------------------------------------------------*/
static const struct PinIntConfig interruptConfig = {
    .pin = EVENT_PIN,
    .event = PIN_FALLING,
    .pull = PIN_PULLUP
};
/*----------------------------------------------------------------------------*/
static void onExternalEvent(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  bool event = false;

  struct Interrupt * const interrupt = init(PinInt, &interruptConfig);
  assert(interrupt);
  interruptSetCallback(interrupt, onExternalEvent, &event);
  interruptEnable(interrupt);

  while (1)
  {
    while (!event)
      barrier();

    pinToggle(led);
    mdelay(10);
    event = false;
  }

  return 0;
}
