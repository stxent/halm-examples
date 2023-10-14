/*
 * lpc11exx_default/shared/board.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/eeprom.h>
#include <halm/platform/lpc/flash.h>
#include <halm/platform/lpc/gptimer.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void)
{
  static const struct GenericClockConfig mainClockConfigExt = {
      .source = CLOCK_EXTERNAL
  };

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainClock, &mainClockConfigExt);
}
/*----------------------------------------------------------------------------*/
void boardSetupClockPll(void)
{
  static const struct PllConfig sysPllConfig = {
      .divisor = 4,
      .multiplier = 16,
      .source = CLOCK_EXTERNAL
  };
  static const struct GenericClockConfig mainClockConfigPll = {
      .source = CLOCK_PLL
  };

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &mainClockConfigPll);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupEeprom(void)
{
  struct Interface * const interface = init(Eeprom, NULL);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupFlash(void)
{
  struct Interface * const interface = init(Flash, NULL);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimer(void)
{
  static const struct GpTimerConfig timerConfig = {
      .frequency = 1000000,
      .event = GPTIMER_MATCH0,
      .channel = GPTIMER_CT32B1
  };

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer != NULL);
  return timer;
}
