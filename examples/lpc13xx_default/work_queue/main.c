/*
 * lpc13xx_default/work_queue/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/core/cortex/systick.h>
#include <halm/generic/work_queue.h>
#include <halm/pin.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN         PIN(3, 0)
#define WORK_QUEUE_SIZE 4
/*----------------------------------------------------------------------------*/
static void blinkTask(void *argument)
{
  static bool value = 0;
  const struct Pin * const pin = argument;

  pinWrite(*pin, value);
  value = !value;
}
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  workQueueAdd(blinkTask, argument);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  /* Initialize peripherals */
  struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  struct Timer * const timer = init(SysTickTimer, 0);
  assert(timer);
  timerSetOverflow(timer, timerGetFrequency(timer) / 2);
  timerSetCallback(timer, onTimerOverflow, &led);

  /* Initialize Work Queue */
  workQueueInit(WORK_QUEUE_SIZE);

  /* Start event generation and queue handler */
  timerEnable(timer);
  workQueueStart(0);

  return 0;
}
