/*
 * lpc17xx_default/counter/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/lpc/gptimer_counter.h>
#include <halm/platform/lpc/gppwm.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define EVENT_PIN   PIN(1, 19)
#define OUTPUT_PIN  PIN(1, 18)
/*----------------------------------------------------------------------------*/
static const struct GpPwmUnitConfig pwmUnitConfig = {
    .frequency = 1000,
    .resolution = 10,
    .channel = 0
};

static const struct GpTimerCounterConfig counterConfig = {
    .edge = PIN_RISING,
    .pin = EVENT_PIN,
    .channel = 1
};
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  bool event = false;

  boardSetupClockPll();

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, true);

  struct GpPwmUnit * const pwmUnit = init(GpPwmUnit, &pwmUnitConfig);
  assert(pwmUnit);
  struct Pwm * const pwmOutput = gpPwmCreate(pwmUnit, OUTPUT_PIN);
  assert(pwmOutput);
  pwmSetDuration(pwmOutput, pwmGetResolution(pwmOutput) / 2);

  struct Timer * const counter = init(GpTimerCounter, &counterConfig);
  assert(counter);

  struct Timer * const timer = boardSetupTimer();
  timerSetOverflow(timer, timerGetFrequency(timer));
  timerSetCallback(timer, onTimerOverflow, &event);

  timerEnable(counter);
  timerEnable(timer);
  pwmEnable(pwmOutput);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    const uint32_t period = timerGetValue(counter);
    timerSetValue(counter, 0);

    if (period >= 999 && period <= 1001)
      pinReset(led);
    else
      pinSet(led);
  }

  return 0;
}
