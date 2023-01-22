/*
 * lpc17xx_default/pm_shutdown/main.c
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/delay.h>
#include <halm/gpio_bus.h>
#include <halm/platform/lpc/backup_domain.h>
#include <halm/platform/lpc/rtc.h>
#include <halm/pm.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
/* Period between wake-ups in seconds */
#define RTC_ALARM_PERIOD  5
/* January 1, 2017, 00:00:00 */
#define RTC_INITIAL_TIME  1483228800

#define MAGIC_WORD        0x3A84508FUL
/*----------------------------------------------------------------------------*/
static const struct SimpleGpioBusConfig busConfig = {
    .pins = (const PinNumber []){
        BOARD_LED_0, BOARD_LED_1, BOARD_LED_2, 0
    },
    .initial = 0,
    .direction = PIN_OUTPUT
};
/*----------------------------------------------------------------------------*/
int main(void)
{
  boardSetupClockPll();

  struct GpioBus * const leds = init(SimpleGpioBus, &busConfig);
  assert(leds);

  uint32_t * const backup = backupDomainAddress();
  const bool restartFlag = backup[0] == MAGIC_WORD;

  const struct RtcConfig rtcConfig = {
      .timestamp = restartFlag ? 0 : RTC_INITIAL_TIME,
      .priority = 0
  };
  struct RtClock * const rtc = init(Rtc, &rtcConfig);
  assert(rtc);
  rtSetAlarm(rtc, rtTime(rtc) + RTC_ALARM_PERIOD);

  uint32_t state = restartFlag ? backup[1] : 1;

  for (unsigned int i = 0; i < 5; ++i)
  {
    gpioBusWrite(leds, i % 2 ? 0 : state);
    mdelay(500);
  }

  gpioBusWrite(leds, 0);
  state = state < 7 ? state + 1 : 1;
  backup[0] = MAGIC_WORD;
  backup[1] = state;

  pmChangeState(PM_SHUTDOWN);

  /* Execution never reaches this point */
  while (1);
  return 0;
}
