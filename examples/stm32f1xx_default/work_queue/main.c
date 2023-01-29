/*
 * stm32f1xx_default/work_queue/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/generic/work_queue.h>
#include <halm/timer.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
static const struct WorkQueueConfig workQueueConfig = {
    .size = 4
};
/*----------------------------------------------------------------------------*/
static void blinkTask(void *argument)
{
  pinToggle(*((struct Pin *)argument));
}
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  wqAdd(WQ_DEFAULT, blinkTask, argument);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  boardSetupClockExt();

  /* Initialize peripherals */
  struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, false);

  struct Timer * const timer = boardSetupTimer();
  timerSetOverflow(timer, timerGetFrequency(timer) / 2);
  timerSetCallback(timer, onTimerOverflow, &led);

  /* Initialize Work Queue */
  WQ_DEFAULT = init(WorkQueue, &workQueueConfig);
  assert(WQ_DEFAULT);

  /* Start event generation and queue handler */
  timerEnable(timer);
  wqStart(WQ_DEFAULT);

  return 0;
}
