/*
 * main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <stdbool.h>

#include <halm/pin.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/pin_interrupt.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN     PIN(3, 0)

#define EVENT_PIN   PIN(1, 0)
#define OUTPUT_PIN  PIN(1, 1)
/*----------------------------------------------------------------------------*/
static const struct GpTimerConfig timerConfig = {
    .frequency = 1000,
    .channel = GPTIMER_CT32B0
};

static const struct PinInterruptConfig eventConfig = {
    .pin = EVENT_PIN,
    .event = PIN_RISING,
    .pull = PIN_PULLDOWN
};
/*----------------------------------------------------------------------------*/
static void onExternalEvent(void *argument)
{
  const struct Pin * const led = argument;

  pinReset(*led);
}
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  bool * const event = argument;

  *event = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  struct Pin led = pinInit(LED_PIN);
  pinOutput(led, 0);

  struct Interrupt * const externalInterrupt = init(PinInterrupt, &eventConfig);
  assert(externalInterrupt);
  intCallback(externalInterrupt, onExternalEvent, &led);

  bool event = false;

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  timerSetOverflow(timer, 100);
  timerCallback(timer, onTimerOverflow, &event);

  const struct Pin output = pinInit(OUTPUT_PIN);
  pinOutput(output, 0);

  intSetEnabled(externalInterrupt, true);
  timerSetEnabled(timer, true);

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
/*----------------------------------------------------------------------------*/
void __assert_func(const char *file __attribute__((unused)),
    int line __attribute__((unused)),
    const char *func __attribute__((unused)),
    const char *expr __attribute__((unused)))
{
  while (1);
}
