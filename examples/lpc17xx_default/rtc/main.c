/*
 * lpc17xx_default/rtc/main.c
 * Copyright (C) 2015 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <xcore/realtime.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
/* Period between wake-ups in seconds */
#define RTC_ALARM_PERIOD 5
/*----------------------------------------------------------------------------*/
struct Context
{
  struct Pin led;
  struct RtClock *rtc;
};
/*----------------------------------------------------------------------------*/
static void onTimerAlarm(void *argument)
{
  struct Context * const context = argument;
  const time64_t currentTime = rtTime(context->rtc);

  rtSetAlarm(context->rtc, currentTime + RTC_ALARM_PERIOD);
  pinToggle(context->led);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  struct Context context;

  boardSetupClockExt();

  context.led = pinInit(BOARD_LED);
  pinOutput(context.led, BOARD_LED_INV);

  pinToggle(context.led);
  context.rtc = boardSetupRtc(false);
  pinToggle(context.led);

  rtSetCallback(context.rtc, onTimerAlarm, &context);
  rtSetAlarm(context.rtc, rtTime(context.rtc) + RTC_ALARM_PERIOD);

  while (1);
  return 0;
}
