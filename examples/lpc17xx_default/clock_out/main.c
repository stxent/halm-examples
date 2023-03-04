/*
 * lpc17xx_default/clock_out/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/gptimer_counter.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define EVENT_PIN   PIN(1, 19)
#define OUTPUT_PIN  PIN(1, 27)
/*----------------------------------------------------------------------------*/
static const struct GpTimerCounterConfig counterConfig = {
    .edge = PIN_RISING,
    .pin = EVENT_PIN,
    .channel = 1
};
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_EXTERNAL
};

static const struct ClockOutputConfig outputClockConfig = {
    .source = CLOCK_MAIN,
    .divisor = 16,
    .pin = OUTPUT_PIN
};
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainClock, &mainClockConfig);

  clockEnable(ClockOutput, &outputClockConfig);
  while (!clockReady(ClockOutput));
}
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  const uint32_t frequency = extOscConfig.frequency / outputClockConfig.divisor;
  const uint32_t maxPeriod = frequency + frequency / 10000;
  const uint32_t minPeriod = frequency - frequency / 10000;

  // TODO
  setupClock();

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, BOARD_LED_INV);

  struct Timer * const counter = init(GpTimerCounter, &counterConfig);
  assert(counter);

  struct Timer * const timer = boardSetupTimer();
  timerSetOverflow(timer, timerGetFrequency(timer));

  bool event = false;
  timerSetCallback(timer, onTimerOverflow, &event);

  timerEnable(counter);
  timerEnable(timer);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    const uint32_t period = timerGetValue(counter);
    timerSetValue(counter, 0);

    if (period >= minPeriod && period <= maxPeriod)
      pinWrite(led, BOARD_LED_INV);
    else
      pinWrite(led, !BOARD_LED_INV);
  }

  return 0;
}
