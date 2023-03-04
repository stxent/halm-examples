/*
 * lpc13xx_default/pwm/main.c
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/pwm.h>
#include <halm/timer.h>
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  static uint32_t iteration = 0;

  struct PwmPackage * const pwm = argument;
  const uint32_t resolution = timerGetOverflow(pwm->timer);

  pwmSetEdges(pwm->outputs[1], 0, iteration % (resolution + 1));
  ++iteration;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  boardSetupClockExt();

  struct PwmPackage pwm = boardSetupPwm();

  timerSetFrequency(pwm.timer, 10000);
  timerSetOverflow(pwm.timer, 10);
  timerEnable(pwm.timer);

  pwmSetEdges(pwm.outputs[0], 0, timerGetOverflow(pwm.timer) / 2);
  pwmEnable(pwm.outputs[0]);
  pwmSetEdges(pwm.outputs[1], 0, 0);
  pwmEnable(pwm.outputs[1]);

  struct Timer * const timer = boardSetupTimer();
  timerSetOverflow(timer, timerGetFrequency(timer));
  timerSetCallback(timer, onTimerOverflow, &pwm);
  timerEnable(timer);

  while (1);
  return 0;
}
