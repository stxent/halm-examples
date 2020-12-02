/*
 * lpc17xx_default/pin_interrupt/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/pin_int.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN     PIN(1, 8)
#define EVENT_PIN   PIN(0, 3)
#define OUTPUT_PIN  PIN(0, 2)
/*----------------------------------------------------------------------------*/
static const struct GpTimerConfig timerConfig = {
    .frequency = 1000,
    .channel = 0
};

static const struct PinIntConfig eventConfig = {
    .pin = EVENT_PIN,
    .event = PIN_RISING,
    .pull = PIN_PULLDOWN
};
/*----------------------------------------------------------------------------*/
static void onExternalEvent(void *argument)
{
  pinReset(*(const struct Pin *)argument);
}
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  struct Interrupt * const externalInterrupt = init(PinInt, &eventConfig);
  assert(externalInterrupt);
  interruptSetCallback(externalInterrupt, onExternalEvent, &led);

  bool event = false;

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  timerSetOverflow(timer, 100);
  timerSetCallback(timer, onTimerOverflow, &event);

  const struct Pin output = pinInit(OUTPUT_PIN);
  pinOutput(output, false);

  interruptEnable(externalInterrupt);
  timerEnable(timer);

  while (1)
  {
    /* First phase */
    while (!event)
      barrier();
    event = false;

    pinSet(led);
    pinSet(output);

    /* Second phase */
    while (!event)
      barrier();
    event = false;

    pinReset(output);
  }

  return 0;
}
