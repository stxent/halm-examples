/*
 * lpc17xx_default/pm_suspend/main.c
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/delay.h>
#include <halm/gpio_bus.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/pm.h>
#include <xcore/realtime.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
/* Period between wake-ups in seconds */
#define RTC_ALARM_PERIOD 5
/*----------------------------------------------------------------------------*/
static const struct SimpleGpioBusConfig busConfig = {
    .pins = (const PinNumber []){
        BOARD_LED_0, BOARD_LED_1, BOARD_LED_2, 0
    },
    .initial = 0,
    .direction = PIN_OUTPUT
};

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct PllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 18,
    .multiplier = 30
};

static const struct GenericClockConfig initialClockConfig = {
    .source = CLOCK_INTERNAL
};

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_PLL
};
/*----------------------------------------------------------------------------*/
static void disableClock(void)
{
  clockEnable(MainClock, &initialClockConfig);

  clockDisable(SystemPll);
  clockDisable(ExternalOsc);
}
/*----------------------------------------------------------------------------*/
static void enableClock(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &mainClockConfig);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  uint32_t state = 1;

  enableClock();

  struct GpioBus * const leds = init(SimpleGpioBus, &busConfig);
  assert(leds);

  struct RtClock * const rtc = boardSetupRtc();
  rtSetAlarm(rtc, rtTime(rtc) + RTC_ALARM_PERIOD);

  while (1)
  {
    for (unsigned int i = 0; i < 5; ++i)
    {
      gpioBusWrite(leds, i % 2 ? 0 : state);
      mdelay(500);
    }

    gpioBusWrite(leds, 0);
    state = state < 7 ? state + 1 : 1;

    disableClock();
    pmChangeState(PM_SUSPEND);
    enableClock();
    pmChangeState(PM_ACTIVE);

    rtSetAlarm(rtc, rtTime(rtc) + RTC_ALARM_PERIOD);
  }

  return 0;
}
