/*
 * bl602_default/shared/board.c
 * Copyright (C) 2026 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/core/riscv/machine_timer.h>
#include <halm/platform/bouffalo/clocking.h>
#include <halm/platform/bouffalo/gptimer.h>
#include <halm/platform/bouffalo/serial.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 40000000
};
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void)
{
  static const struct GenericClockConfig mainClockConfigExt = {
      .divisor = 1,
      .source = CLOCK_EXTERNAL
  };

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainClock, &mainClockConfigExt);
}
/*----------------------------------------------------------------------------*/
void boardSetupClockPll(void)
{
  static const struct GenericClockConfig mainClockConfigInt = {
      .divisor = 1,
      .source = CLOCK_INTERNAL
  };
  static const struct GenericClockConfig mainClockConfigPll = {
      .divisor = 1,
      .source = CLOCK_PLL_160MHZ
  };

  clockEnable(MainClock, &mainClockConfigInt);

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &(struct PllConfig){CLOCK_EXTERNAL});
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &mainClockConfigPll);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerial(void)
{
  static const struct SerialConfig serialConfig = {
      .rxLength = BOARD_UART_BUFFER,
      .txLength = BOARD_UART_BUFFER,
      .rate = 19200,
      .rx = PIN(0, 7),
      .tx = PIN(0, 16),
      .channel = 0
  };
  static const struct GenericClockConfig uartClockConfig = {
      .divisor = 1,
      .source = CLOCK_SYSTEM
  };

  clockEnable(UartClock, &uartClockConfig);
  while (!clockReady(UartClock));

  struct Interface * const interface = init(Serial, &serialConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimer(void)
{
  static const struct GpTimerConfig timerConfig = {
      .frequency = 1000000,
      .channel = TIM2
  };

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Timer64 *boardSetupTimer64(void)
{
  struct Timer64 * const timer = init(MachineTimer64, NULL);
  assert(timer != NULL);
  return timer;
}
