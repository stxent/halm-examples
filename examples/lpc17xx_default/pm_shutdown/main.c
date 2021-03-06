/*
 * lpc17xx_default/pm_shutdown/main.c
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/delay.h>
#include <halm/gpio_bus.h>
#include <halm/pin.h>
#include <halm/platform/lpc/backup_domain.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/rtc.h>
#include <halm/pm.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
/* Period between wake-ups in seconds */
#define RTC_ALARM_PERIOD  5
/* January 1, 2017, 00:00:00 */
#define RTC_INITIAL_TIME  1483228800

#define LED_PIN_0 PIN(1, 8)
#define LED_PIN_1 PIN(1, 9)
#define LED_PIN_2 PIN(1, 10)

#define MAGIC_WORD 0x3A84508FUL
/*----------------------------------------------------------------------------*/
static const struct SimpleGpioBusConfig busConfig = {
    .pins = (const PinNumber []){
        LED_PIN_0, LED_PIN_1, LED_PIN_2, 0
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

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_PLL
};
/*----------------------------------------------------------------------------*/
static void setupClock(void)
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
  setupClock();

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
