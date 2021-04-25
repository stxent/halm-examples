/*
 * lpc43xx_default/pin_interrupt/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/gptimer.h>
#include <halm/platform/lpc/pin_int.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN     PIN(PORT_6, 6)
#define EVENT_PIN   PIN(PORT_3, 1)
#define OUTPUT_PIN  PIN(PORT_3, 2)
/*----------------------------------------------------------------------------*/
static const struct PinIntConfig eventConfig = {
    .pin = EVENT_PIN,
    .event = PIN_RISING,
    .pull = PIN_PULLDOWN
};

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_INTERNAL
};

static const struct GpTimerConfig timerConfig = {
    .frequency = 1000,
    .channel = 0
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
