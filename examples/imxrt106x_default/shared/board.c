/*
 * imxrt106x_default/shared/board.c
 * Copyright (C) 2024 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/imxrt/clocking.h>
#include <halm/platform/imxrt/serial.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
[[gnu::alias("boardSetupClockPll3")]] void boardSetupClockPll(void);
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void)
{
  static const struct ExtendedClockConfig periphClockConfig = {
      .divisor = 1,
      .source = CLOCK_OSC
  };

  clockEnable(PeriphClock, &periphClockConfig);
  while (!clockReady(PeriphClock));

  clockEnable(ExternalOsc, NULL);
  while (!clockReady(ExternalOsc));
  clockDisable(InternalOsc);

  clockEnable(MainClock, &(struct GenericClockConfig){1});
  while (!clockReady(MainClock));
}
/*----------------------------------------------------------------------------*/
void boardSetupClockInt(void)
{
  static const struct ExtendedClockConfig periphClockConfig = {
      .divisor = 1,
      .source = CLOCK_OSC
  };

  clockEnable(PeriphClock, &periphClockConfig);
  while (!clockReady(PeriphClock));

  clockEnable(InternalOsc, NULL);
  while (!clockReady(InternalOsc));
  clockDisable(ExternalOsc);

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
/*----------------------------------------------------------------------------*/
void boardSetupClockPll3(void)
{
  static const struct ExtendedClockConfig flexSpi1ClockConfig = {
      .divisor = 8,
      .source = CLOCK_SYSTEM_PLL_PFD2
  };
  static const struct ExtendedClockConfig periphClockConfig = {
      .divisor = 1,
      .source = CLOCK_USB1_PLL
  };

  boardSetupClockExt();

  /* Make 396 MHz clock for FlexSPI1 */
  clockEnable(SystemPll, &(struct PllConfig){22});
  while (!clockReady(SystemPll));
  clockEnable(SystemPllPfd2, &(struct PllConfig){24});
  while (!clockReady(SystemPllPfd2));
  clockEnable(FlexSpi1Clock, &flexSpi1ClockConfig);
  while (!clockReady(FlexSpi1Clock));

  /* Make 480 MHz clock for core */
  clockEnable(Usb1Pll, &(struct PllConfig){20});
  while (!clockReady(Usb1Pll));
  clockEnable(PeriphClock, &periphClockConfig);
  while (!clockReady(PeriphClock));
  clockEnable(MainClock, &(struct GenericClockConfig){1});
  while (!clockReady(MainClock));
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerial(void)
{
  static const struct SerialConfig serialConfig = {
      .rxLength = BOARD_UART_BUFFER,
      .txLength = BOARD_UART_BUFFER,
      .rate = 19200,
      .rx = PIN(PORT_AD_B1, 3),
      .tx = PIN(PORT_AD_B1, 2),
      .channel = 1 /* LPUART2 */
  };
  static const struct ExtendedClockConfig uartClockConfig = {
      .divisor = 1,
      .source = CLOCK_USB1_PLL
  };

  clockEnable(UartClock, &uartClockConfig);
  while (!clockReady(UartClock));

  struct Interface * const interface = init(Serial, &serialConfig);
  assert(interface != NULL);
  return interface;
}
