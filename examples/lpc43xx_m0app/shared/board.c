/*
 * lpc43xx_m0app/shared/board.c
 * Copyright (C) 2024 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/gptimer.h>
#include <halm/platform/lpc/serial.h>
#include <halm/platform/lpc/usb_device.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
[[gnu::alias("boardSetupClock")]] void boardSetupClockExt(void);
[[gnu::alias("boardSetupClock")]] void boardSetupClockPll(void);

static void enablePeriphClock(const void *);
/*----------------------------------------------------------------------------*/
[[gnu::section(".shared")]] static struct ClockSettings sharedClockSettings;
/*----------------------------------------------------------------------------*/
static void enablePeriphClock(const void *clock)
{
  if (clockReady(clock))
    clockDisable(clock);

  if (clockReady(SystemPll))
    clockEnable(clock, &(struct GenericClockConfig){CLOCK_PLL});
  else if (clockReady(ExternalOsc))
    clockEnable(clock, &(struct GenericClockConfig){CLOCK_EXTERNAL});
  else
    clockEnable(clock, &(struct GenericClockConfig){CLOCK_INTERNAL});

  while (!clockReady(clock));
}
/*----------------------------------------------------------------------------*/
void boardSetupClock(void)
{
  loadClockSettings(&sharedClockSettings);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerial(void)
{
  static const struct SerialConfig serialConfig = {
      .rxLength = BOARD_UART_BUFFER,
      .txLength = BOARD_UART_BUFFER,
      .rate = 19200,
      .rx = PIN(PORT_1, 14),
      .tx = PIN(PORT_5, 6),
      .channel = 1
  };

  if (serialConfig.channel == 0)
    enablePeriphClock(Usart0Clock);
  else if (serialConfig.channel == 1)
    enablePeriphClock(Uart1Clock);
  else if (serialConfig.channel == 2)
    enablePeriphClock(Usart2Clock);
  else
    enablePeriphClock(Usart3Clock);

  struct Interface * const interface = init(Serial, &serialConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimer(void)
{
  static const struct GpTimerConfig timerConfig = {
      .frequency = 1000000,
      .event = GPTIMER_MATCH0,
      .channel = 3
  };

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Usb *boardSetupUsb(void)
{
  /* Clocks */
  static const struct PllConfig audioPllConfig = {
      .divisor = 8,
      .multiplier = 40,
      .source = CLOCK_EXTERNAL
  };

  /* Objects */
  static const struct UsbDeviceConfig usb1Config = {
      .dm = PIN(PORT_USB, PIN_USB1_DM),
      .dp = PIN(PORT_USB, PIN_USB1_DP),
      .connect = 0,
      .vbus = PIN(PORT_2, 5),
      .vid = 0x15A2,
      .pid = 0x0044,
      .channel = 1
  };

  /* Make 60 MHz clock on AUDIO PLL for USB1 */
  if (!clockReady(AudioPll))
  {
    clockEnable(AudioPll, &audioPllConfig);
    while (!clockReady(AudioPll));
  }

  clockEnable(Usb1Clock, &(struct GenericClockConfig){CLOCK_AUDIO_PLL});
  while (!clockReady(Usb1Clock));

  struct Usb * const usb = init(UsbDevice, &usb1Config);
  assert(usb != NULL);
  return usb;
}
