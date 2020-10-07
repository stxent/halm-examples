/*
 * stm32f1xx_default/work_queue/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/core/cortex/systick.h>
#include <halm/generic/work_queue.h>
#include <halm/pin.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN PIN(PORT_C, 13)
/*----------------------------------------------------------------------------*/
static const struct WorkQueueConfig workQueueConfig = {
    .size = 4
};
/*----------------------------------------------------------------------------*/
static void blinkTask(void *argument)
{
  const struct Pin * const pin = argument;
  pinToggle(*pin);
}
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  wqAdd(WQ_DEFAULT, blinkTask, argument);
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
  WQ_DEFAULT = init(WorkQueue, &workQueueConfig);
  assert(WQ_DEFAULT);

  /* Start event generation and queue handler */
  timerEnable(timer);
  wqStart(WQ_DEFAULT);

  return 0;
}
