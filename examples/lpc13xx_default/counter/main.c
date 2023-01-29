/*
 * lpc13xx_default/counter/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/pwm.h>
#include <halm/timer.h>
#include <xcore/memory.h>
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  bool event = false;

  boardSetupClockExt();

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, true);

  struct Timer * const counterTimer = boardSetupCounterTimer();
  timerEnable(counterTimer);

  struct Timer * const eventTimer = boardSetupTimer();
  timerSetOverflow(eventTimer, timerGetFrequency(eventTimer));
  timerSetCallback(eventTimer, onTimerOverflow, &event);
  timerEnable(eventTimer);

  struct PwmPackage pwm = boardSetupPwm();
  pwmSetEdges(pwm.output, 0, timerGetOverflow(pwm.timer) / 2);
  pwmEnable(pwm.output);
  timerEnable(pwm.timer);

  const uint32_t PWM_FREQUENCY =
      timerGetFrequency(pwm.timer) / timerGetOverflow(pwm.timer);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    const uint32_t period = timerGetValue(counterTimer);
    timerSetValue(counterTimer, 0);

    if (period >= PWM_FREQUENCY - 1 && period <= PWM_FREQUENCY + 1)
      pinReset(led);
    else
      pinSet(led);
  }

  return 0;
}
