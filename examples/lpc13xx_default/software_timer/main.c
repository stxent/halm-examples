/*
 * lpc13xx_default/software_timer/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/generic/software_timer.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
static void onTimer0Overflow(void *argument)
{
  static bool state = false;

  pinWrite(*(const struct Pin *)argument, state);
  state = !state;
}
/*----------------------------------------------------------------------------*/
static void onTimer1Overflow(void *argument)
{
  static bool state = false;

  pinWrite(*(const struct Pin *)argument, state);
  state = !state;
}
/*----------------------------------------------------------------------------*/
static void onTimer2Overflow(void *argument)
{
  static bool state = false;

  pinWrite(*(const struct Pin *)argument, state);
  state = !state;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  boardSetupClockPll();

  struct Pin output0 = pinInit(BOARD_LED_0);
  pinOutput(output0, false);
  struct Pin output1 = pinInit(BOARD_LED_1);
  pinOutput(output1, false);
  struct Pin output2 = pinInit(BOARD_LED_2);
  pinOutput(output2, false);

  struct Timer * const tickTimer = boardSetupTimer();
  timerSetOverflow(tickTimer, timerGetFrequency(tickTimer) / 1000);

  const struct SoftwareTimerFactoryConfig factoryConfig = {
      .timer = tickTimer
  };

  struct SoftwareTimerFactory * const timerFactory =
      init(SoftwareTimerFactory, &factoryConfig);
  assert(timerFactory);

  struct Timer * const timer0 = softwareTimerCreate(timerFactory);
  assert(timer0);
  timerSetCallback(timer0, onTimer0Overflow, &output0);
  timerSetOverflow(timer0, 199);
  timerEnable(timer0);

  struct Timer * const timer1 = softwareTimerCreate(timerFactory);
  assert(timer1);
  timerSetCallback(timer1, onTimer1Overflow, &output1);
  timerSetOverflow(timer1, 37);
  timerEnable(timer1);

  struct Timer * const timer2 = softwareTimerCreate(timerFactory);
  assert(timer2);
  timerSetCallback(timer2, onTimer2Overflow, &output2);
  timerSetOverflow(timer2, 97);
  timerEnable(timer2);

  timerEnable(tickTimer);

  while (1);
  return 0;
}
