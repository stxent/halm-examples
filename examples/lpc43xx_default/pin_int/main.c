/*
 * lpc43xx_default/pin_int/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/delay.h>
#include <halm/pin.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/pin_int.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define EVENT_PIN PIN(PORT_3, 1)
#define LED_PIN   PIN(PORT_7, 7)
/*----------------------------------------------------------------------------*/
static const struct PinIntConfig interruptConfig = {
    .pin = EVENT_PIN,
    .event = PIN_FALLING,
    .pull = PIN_PULLUP
};

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_INTERNAL
};
/*----------------------------------------------------------------------------*/
static void onExternalEvent(void *argument)
{
  *(bool *)argument = true;
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