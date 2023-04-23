/*
 * lpc43xx_default/pm_suspend/main.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/delay.h>
#include <halm/pm.h>
#include <xcore/realtime.h>
/*----------------------------------------------------------------------------*/
/* Period between wake-ups in seconds */
#define RTC_ALARM_PERIOD 10
/*----------------------------------------------------------------------------*/
int main(void)
{
  boardSetupClockPll();

  const struct Pin leds[] = {
      pinInit(BOARD_LED_0),
      pinInit(BOARD_LED_1)
  };
  pinOutput(leds[0], BOARD_LED_INV);
  pinOutput(leds[1], BOARD_LED_INV);

  /* Non-blocking initialization */
  struct RtClock * const rtc = boardSetupRtc(false);
  /* Wait for RTC registers update */
  while (rtTime(rtc) == 0);
  rtSetAlarm(rtc, rtTime(rtc) + RTC_ALARM_PERIOD);

  uint32_t state = 0;

  while (1)
  {
    const unsigned int phase = (state % 3) + 1;

    for (unsigned int i = 0; i < 5; ++i)
    {
      if (phase & 0x01)
        pinToggle(leds[0]);
      if (phase & 0x02)
        pinToggle(leds[1]);

      mdelay(500);
    }

    ++state;

    /* Disable clocks and reset LED */
    pinWrite(leds[0], BOARD_LED_INV);
    pinWrite(leds[1], BOARD_LED_INV);
    boardResetClock();

    /* Enter power-down mode */
    pmChangeState(PM_SUSPEND);

    /* Enable clocks and notify all drivers */
    boardSetupClockPll();
    pmChangeState(PM_ACTIVE);

    rtSetAlarm(rtc, rtTime(rtc) + RTC_ALARM_PERIOD);
  }

  return 0;
}
