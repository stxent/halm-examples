/*
 * lpc17xx_default/rtc/main.c
 * Copyright (C) 2015 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/lpc/rtc.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
/* Period between wake-ups in seconds */
#define RTC_ALARM_PERIOD  5
/* January 1, 2017, 00:00:00 */
#define RTC_INITIAL_TIME  1483228800

#define LED_PIN           PIN(1, 8)
/*----------------------------------------------------------------------------*/
struct Context
{
  struct Pin led;
  struct RtClock *rtc;
};
/*----------------------------------------------------------------------------*/
static const struct RtcConfig rtcConfig = {
    .timestamp = RTC_INITIAL_TIME
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

  context.led = pinInit(LED_PIN);
  pinOutput(context.led, false);

  pinSet(context.led);
  context.rtc = init(Rtc, &rtcConfig);
  assert(context.rtc);
  rtSetCallback(context.rtc, onTimerAlarm, &context);
  pinReset(context.led);

  rtSetAlarm(context.rtc, rtTime(context.rtc) + RTC_ALARM_PERIOD);

  while (1);
  return 0;
}
