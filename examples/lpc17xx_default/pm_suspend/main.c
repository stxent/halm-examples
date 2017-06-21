/*
 * lpc17xx_default/pm_suspend/main.c
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/delay.h>
#include <halm/gpio_bus.h>
#include <halm/pin.h>
#include <halm/platform/nxp/lpc17xx/clocking.h>
#include <halm/platform/nxp/rtc.h>
#include <halm/pm.h>
/*----------------------------------------------------------------------------*/
/* Period between wake-ups in seconds */
#define RTC_ALARM_PERIOD  5
/* January 1, 2017, 00:00:00 */
#define RTC_INITIAL_TIME  1483228800

#define LED_PIN_0 PIN(1, 8)
#define LED_PIN_1 PIN(1, 9)
#define LED_PIN_2 PIN(1, 10)
/*----------------------------------------------------------------------------*/
static const struct SimpleGpioBusConfig busConfig = {
    .pins = (const PinNumber []){
        LED_PIN_0, LED_PIN_1, LED_PIN_2, 0
    },
    .initial = 0,
    .direction = PIN_OUTPUT
};

static const struct RtcConfig rtcConfig = {
    .timestamp = RTC_INITIAL_TIME
};

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct PllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 18,
    .multiplier = 30
};

static const struct CommonClockConfig intClockSource = {
    .source = CLOCK_INTERNAL
};

static const struct CommonClockConfig pllClockSource = {
    .source = CLOCK_PLL
};
/*----------------------------------------------------------------------------*/
static void disableClock(void)
{
  clockEnable(MainClock, &intClockSource);

  clockDisable(SystemPll);
  clockDisable(ExternalOsc);
}
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &pllClockSource);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

  struct GpioBus * const leds = init(SimpleGpioBus, &busConfig);
  assert(leds);

  struct RtClock * const rtc = init(Rtc, &rtcConfig);
  assert(rtc);
  rtSetAlarm(rtc, rtTime(rtc) + RTC_ALARM_PERIOD);

  uint32_t state = 1;

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
    setupClock();
    pmChangeState(PM_ACTIVE);

    rtSetAlarm(rtc, rtTime(rtc) + RTC_ALARM_PERIOD);
  }

  return 0;
}
/*----------------------------------------------------------------------------*/
void __assert_func(const char *file __attribute__((unused)),
    int line __attribute__((unused)),
    const char *func __attribute__((unused)),
    const char *expr __attribute__((unused)))
{
  while (1);
}
