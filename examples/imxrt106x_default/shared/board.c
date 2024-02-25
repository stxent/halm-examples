/*
 * imxrt106x_default/shared/board.c
 * Copyright (C) 2024 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/imxrt/clocking.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
void boardSetupClockPll(void) __attribute__((alias("boardSetupClockPll1")));
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void)
{
  static const struct ExtendedClockConfig periphClockConfig = {
      .divisor = 1,
      .source = CLOCK_OSC
  };

  clockEnable(PeriphClock, &periphClockConfig);
  while (!clockReady(PeriphClock));

  clockEnable(MainClock, &(struct GenericClockConfig){1});
  while (!clockReady(MainClock));
}
/*----------------------------------------------------------------------------*/
void boardSetupClockPll1(void)
{
  static const struct ExtendedClockConfig periphClockConfig = {
      .divisor = 2,
      .source = CLOCK_ARM_PLL
  };

  boardSetupClockExt();

  clockEnable(ArmPll, &(struct PllConfig){100});
  while (!clockReady(ArmPll));

  clockEnable(PeriphClock, &periphClockConfig);
  while (!clockReady(PeriphClock));

  clockEnable(MainClock, &(struct GenericClockConfig){1});
  while (!clockReady(MainClock));
}
/*----------------------------------------------------------------------------*/
void boardSetupClockPll2Pfd0(void)
{
  static const struct ExtendedClockConfig periphClockConfig = {
      .divisor = 1,
      .source = CLOCK_SYSTEM_PLL_PFD0
  };

  boardSetupClockExt();

  clockEnable(SystemPll, &(struct PllConfig){22});
  while (!clockReady(SystemPll));
  clockEnable(SystemPllPfd0, &(struct PllConfig){24});
  while (!clockReady(SystemPllPfd0));

  clockEnable(PeriphClock, &periphClockConfig);
  while (!clockReady(PeriphClock));

  clockEnable(MainClock, &(struct GenericClockConfig){1});
  while (!clockReady(MainClock));
}
