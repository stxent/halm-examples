/*
 * imxrt106x_default/shared/board.c
 * Copyright (C) 2024 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/imxrt/clocking.h>
#include <halm/platform/imxrt/pit.h>
#include <halm/platform/imxrt/serial.h>
#include <halm/platform/imxrt/serial_dma.h>
#include <halm/platform/imxrt/usb_device.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
[[gnu::alias("boardSetupClockPll3")]] void boardSetupClockPll(void);

[[gnu::alias("boardSetupTimerPIT3")]] struct Timer *boardSetupTimer(void);
[[gnu::alias("boardSetupTimerPIT0")]] struct Timer *boardSetupTimerPIT(void);

[[gnu::alias("boardSetupUsb1")]] struct Entity *boardSetupUsb(void);
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void)
{
  static const struct ExtendedClockConfig periphClockConfig = {
      .divisor = 1,
      .source = CLOCK_OSC
  };
  static const struct ExtendedClockConfig timerClockConfig = {
      .divisor = 1,
      .source = CLOCK_IPG
  };

  clockEnable(PeriphClock, &periphClockConfig);
  while (!clockReady(PeriphClock));

  clockEnable(IpgClock, &(struct GenericClockConfig){1});
  while (!clockReady(IpgClock));

  clockEnable(TimerClock, &timerClockConfig);
  while (!clockReady(TimerClock));

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
  static const struct ExtendedClockConfig timerClockConfig = {
      .divisor = 1,
      .source = CLOCK_IPG
  };

  clockEnable(PeriphClock, &periphClockConfig);
  while (!clockReady(PeriphClock));

  clockEnable(IpgClock, &(struct GenericClockConfig){1});
  while (!clockReady(IpgClock));

  clockEnable(TimerClock, &timerClockConfig);
  while (!clockReady(TimerClock));

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
  static const struct ExtendedClockConfig timerClockConfig = {
      .divisor = 2,
      .source = CLOCK_IPG
  };

  boardSetupClockExt();

  clockEnable(ArmPll, &(struct PllConfig){100});
  while (!clockReady(ArmPll));

  clockEnable(PeriphClock, &periphClockConfig);
  while (!clockReady(PeriphClock));

  clockEnable(IpgClock, &(struct GenericClockConfig){4});
  while (!clockReady(IpgClock));

  clockEnable(TimerClock, &timerClockConfig);
  while (!clockReady(TimerClock));

  /* Make 600 MHz clock for core */
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
  static const struct ExtendedClockConfig timerClockConfig = {
      .divisor = 2,
      .source = CLOCK_IPG
  };

  boardSetupClockExt();

  clockEnable(SystemPll, &(struct PllConfig){22});
  while (!clockReady(SystemPll));
  clockEnable(SystemPllPfd0, &(struct PllConfig){24});
  while (!clockReady(SystemPllPfd0));

  clockEnable(PeriphClock, &periphClockConfig);
  while (!clockReady(PeriphClock));

  clockEnable(IpgClock, &(struct GenericClockConfig){3});
  while (!clockReady(IpgClock));

  clockEnable(TimerClock, &timerClockConfig);
  while (!clockReady(TimerClock));

  /* Make 396 MHz clock for core */
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
  static const struct ExtendedClockConfig timerClockConfig = {
      .divisor = 2,
      .source = CLOCK_IPG
  };

  boardSetupClockExt();

  /* Prepare other system clocks */
  clockEnable(IpgClock, &(struct GenericClockConfig){4});
  while (!clockReady(IpgClock));
  clockEnable(TimerClock, &timerClockConfig);
  while (!clockReady(TimerClock));

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

/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerialDma(void)
{
  static const struct SerialDmaConfig serialDmaConfig = {
      .rxChunk = BOARD_UART_BUFFER / 4,
      .rxLength = BOARD_UART_BUFFER,
      .txLength = BOARD_UART_BUFFER,
      .rate = 19200,
      .rx = PIN(PORT_AD_B1, 3),
      .tx = PIN(PORT_AD_B1, 2),
      .channel = 1, /* LPUART2 */
      .dma = {0, 1}
  };
  static const struct ExtendedClockConfig uartClockConfig = {
      .divisor = 1,
      .source = CLOCK_USB1_PLL
  };

  clockEnable(UartClock, &uartClockConfig);
  while (!clockReady(UartClock));

  struct Interface * const interface = init(SerialDma, &serialDmaConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimerPIT0(void)
{
  static const struct PitConfig pitConfig = {
      .frequency = 1000000,
      .channel = 0,
      .chain = true
  };

  struct Timer * const timer = init(Pit, &pitConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimerPIT2(void)
{
  static const struct PitConfig pitConfig = {
      .channel = 2
  };

  struct Timer * const timer = init(Pit, &pitConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimerPIT3(void)
{
  static const struct PitConfig pitConfig = {
      .channel = 3
  };

  struct Timer * const timer = init(Pit, &pitConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Entity *boardSetupUsb1(void)
{
  /* Objects */
  static const struct UsbDeviceConfig usb1Config = {
      .dm = PIN(PORT_USB, PIN_USB_OTG1_DN),
      .dp = PIN(PORT_USB, PIN_USB_OTG1_DP),
      .vbus = PIN(PORT_USB, PIN_USB_OTG1_VBUS),
      .vid = 0x15A2,
      .pid = 0x0044,
      .channel = USB_OTG1
  };

  assert(clockReady(Usb1Pll));

  struct Entity * const usb = init(UsbDevice, &usb1Config);
  assert(usb != NULL);
  return usb;
}

/*----------------------------------------------------------------------------*/
struct Entity *boardSetupUsb2(void)
{
  /* Objects */
  static const struct UsbDeviceConfig usb2Config = {
      .dm = PIN(PORT_USB, PIN_USB_OTG2_DN),
      .dp = PIN(PORT_USB, PIN_USB_OTG2_DP),
      .vbus = PIN(PORT_USB, PIN_USB_OTG2_VBUS),
      .vid = 0x15A2,
      .pid = 0x0044,
      .channel = USB_OTG2
  };

  /* Make 480 MHz clock for USB OTG2 */
  clockEnable(Usb2Pll, &(struct PllConfig){20});
  while (!clockReady(Usb2Pll));

  struct Entity * const usb = init(UsbDevice, &usb2Config);
  assert(usb != NULL);
  return usb;
}
