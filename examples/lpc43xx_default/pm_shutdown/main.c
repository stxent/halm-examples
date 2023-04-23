/*
 * lpc43xx_default/pm_shutdown/main.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/delay.h>
#include <halm/platform/lpc/backup_domain.h>
#include <halm/pm.h>
#include <xcore/realtime.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
/* Period between wake-ups in seconds */
#define RTC_ALARM_PERIOD  10

#define MAGIC_WORD        0x3A84508FUL
/*----------------------------------------------------------------------------*/
int main(void)
{
  boardSetupClockExt();

  const struct Pin leds[] = {
      pinInit(BOARD_LED_0),
      pinInit(BOARD_LED_1)
  };
  pinOutput(leds[0], BOARD_LED_INV);
  pinOutput(leds[1], BOARD_LED_INV);

  uint32_t * const backup = backupDomainAddress();
  const bool restartFlag = backup[0] == MAGIC_WORD;
  const uint32_t state = restartFlag ? backup[1] : 0;
  const unsigned int phase = (state % 3) + 1;

  /* Non-blocking initialization */
  struct RtClock * const rtc = boardSetupRtc(restartFlag);
  /* Wait for RTC registers update */
  while (rtTime(rtc) == 0);
  rtSetAlarm(rtc, rtTime(rtc) + RTC_ALARM_PERIOD);

  for (unsigned int i = 0; i < 5; ++i)
  {
    if (phase & 0x01)
      pinToggle(leds[0]);
    if (phase & 0x02)
      pinToggle(leds[1]);

    mdelay(500);
  }

  /* Save current state */
  backup[0] = MAGIC_WORD;
  backup[1] = state + 1;

  /* Enter deep power-down mode */
  pmChangeState(PM_SHUTDOWN);

  /* Execution never reaches this point */
  while (1);
  return 0;
}
