/*
 * lpc43xx_default/rtc/main.c
 * Copyright (C) 2015 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/nxp/lpc43xx/clocking.h>
#include <halm/platform/nxp/rtc.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
/* Period between wake-ups in seconds */
#define RTC_ALARM_PERIOD  5
/* January 1, 2015, 00:00:00 */
#define RTC_INITIAL_TIME  1483228800

#define LED_PIN           PIN(PORT_6, 6)
/*----------------------------------------------------------------------------*/
static struct RtClock *rtc;
static struct Pin led;
/*----------------------------------------------------------------------------*/
static const struct RtcConfig rtcConfig = {
    .timestamp = RTC_INITIAL_TIME
};
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(RtcOsc, 0);
  while (!clockReady(RtcOsc));
}
/*----------------------------------------------------------------------------*/
static void onTimerAlarm(void *argument __attribute__((unused)))
{
  static bool value = true;
  const time64_t currentTime = rtTime(rtc);

  rtSetAlarm(rtc, currentTime + RTC_ALARM_PERIOD);

  pinWrite(led, value);
  value = !value;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

  led = pinInit(LED_PIN);
  pinOutput(led, false);

  pinSet(led);
  rtc = init(Rtc, &rtcConfig);
  assert(rtc);
  rtSetCallback(rtc, onTimerAlarm, 0);
  pinReset(led);

  rtSetAlarm(rtc, rtTime(rtc) + RTC_ALARM_PERIOD);

  while (1);
  return 0;
}
