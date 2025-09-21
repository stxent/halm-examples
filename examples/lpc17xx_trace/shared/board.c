/*
 * lpc17xx_trace/shared/board.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/generic/sdio_spi.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/gptimer.h>
#include <halm/platform/lpc/serial.h>
#include <halm/platform/lpc/spi_dma.h>
#include <halm/platform/lpc/usb_device.h>
#include <halm/usb/usb_trace.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
[[gnu::alias("boardSetupTimer1")]] struct Timer *boardSetupAdcTimer(void);
[[gnu::alias("boardSetupTimer0")]] struct Timer *boardSetupTimer(void);
[[gnu::alias("boardSetupTimer1")]] struct Timer *boardSetupTimerAux(void);

[[gnu::alias("boardSetupSpi0")]] struct Interface *boardSetupSpiSdio(void);

[[gnu::alias("boardSetupSpi0")]] struct Interface *boardSetupSpi(void);
/*----------------------------------------------------------------------------*/
DECLARE_WQ_IRQ(WQ_LP, SPI_ISR)
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void)
{
  static const struct ExternalOscConfig extOscConfig = {
      .frequency = 12000000
  };

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainClock, &(struct GenericClockConfig){CLOCK_EXTERNAL});
}
/*----------------------------------------------------------------------------*/
void boardSetupClockPll(void)
{
  static const struct ExternalOscConfig extOscConfig = {
      .frequency = 12000000
  };
  static const struct PllConfig sysPllConfig = {
      .divisor = 4,
      .multiplier = 32,
      .source = CLOCK_EXTERNAL
  };

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &(struct GenericClockConfig){CLOCK_PLL});
}
/*----------------------------------------------------------------------------*/
void boardSetupLowPriorityWQ(void)
{
  static const struct WorkQueueIrqConfig wqIrqConfig = {
      .size = 4,
      .irq = SPI_IRQ,
      .priority = 0
  };

  WQ_LP = init(WorkQueueIrq, &wqIrqConfig);
  assert(WQ_LP != NULL);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSdio(bool, struct Timer *timer)
{
  static const size_t SDIO_MAX_BLOCKS = 32768 >> 9; /* RAM size / block size */
  static const uint32_t SDIO_POLL_RATE = 5000;
  static const uint8_t SPI_SDIO_MODE = 3;

  /* Configure helper timer for SDIO status polling */
  if (timer != NULL)
  {
    assert(timerGetFrequency(timer) >= 10 * SDIO_POLL_RATE);
    timerSetOverflow(timer, timerGetFrequency(timer) / SDIO_POLL_RATE);
  }

  struct Interface *sdio;
  struct Interface *spi;
  [[maybe_unused]] enum Result res;

  /* Initialize and start a Work Queue for CRC computation */
  boardSetupLowPriorityWQ();
  wqStart(WQ_LP);

  /* Initialize SPI layer */
  spi = boardSetupSpiSdio();
  assert(spi != NULL);
  res = ifSetParam(spi, IF_SPI_MODE, &SPI_SDIO_MODE);
  assert(res == E_OK);

  /* Initialize SDIO layer */
  const struct SdioSpiConfig sdioSpiConfig = {
      .interface = spi,
      .timer = timer,
      .wq = WQ_LP,
      .blocks = SDIO_MAX_BLOCKS,
      .cs = BOARD_SDIO_CS
  };
  sdio = init(SdioSpi, &sdioSpiConfig);
  assert(sdio != NULL);

  return sdio;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerial(void)
{
  static const struct SerialConfig serialConfig = {
      .rxLength = 16,
      .txLength = BOARD_UART_BUFFER,
      .rate = 1500000,
      .rx = PIN(0, 16),
      .tx = PIN(0, 15),
      .channel = 1
  };

  struct Interface * const interface = init(Serial, &serialConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpi0(void)
{
  static const struct SpiDmaConfig spiDmaConfig = {
      .rate = 2000000,
      .miso = PIN(0, 17),
      .mosi = PIN(0, 18),
      .sck = PIN(1, 20),
      .channel = 0,
      .mode = 3,
      .dma = {0, 1}
  };

  struct Interface * const interface = init(SpiDma, &spiDmaConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpi1(void)
{
  static const struct SpiDmaConfig spiDmaConfig = {
      .rate = 2000000,
      .miso = PIN(0, 8),
      .mosi = PIN(0, 9),
      .sck = PIN(0, 7),
      .channel = 1,
      .mode = 3,
      .dma = {0, 1}
  };

  struct Interface * const interface = init(SpiDma, &spiDmaConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimer0(void)
{
  static const struct GpTimerConfig timerConfig = {
      .frequency = 1000000,
      .event = GPTIMER_MATCH0,
      .channel = 0
  };

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimer1(void)
{
  static const struct GpTimerConfig timerConfig = {
      .frequency = 1000000,
      .event = GPTIMER_MATCH1, /* Used as an ADC trigger */
      .channel = 1
  };

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimer2(void)
{
  static const struct GpTimerConfig timerConfig = {
      .frequency = 1000000,
      .event = GPTIMER_MATCH0,
      .channel = 2
  };

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Usb *boardSetupUsb(void)
{
  /* Clocks */
  static const struct GenericClockConfig usbClockConfig = {
      .source = CLOCK_USB_PLL
  };
  static const struct PllConfig usbPllConfig = {
      .divisor = 4,
      .multiplier = 16,
      .source = CLOCK_EXTERNAL
  };

  /* Objects */
  static const struct UsbDeviceConfig usbConfig = {
      .dm = PIN(0, 30),
      .dp = PIN(0, 29),
      .connect = PIN(2, 9),
      .vbus = PIN(1, 30),
      .vid = 0x15A2,
      .pid = 0x0044,
      .channel = 0
  };

  clockEnable(UsbPll, &usbPllConfig);
  while (!clockReady(UsbPll));

  clockEnable(UsbClock, &usbClockConfig);
  while (!clockReady(UsbClock));

  struct Timer * const traceChrono = boardSetupTimer2();
  struct Interface * const traceSerial = boardSetupSerial();

  usbTraceInit(traceSerial, traceChrono);

  struct Usb * const usb = init(UsbDevice, &usbConfig);
  assert(usb != NULL);
  return usb;
}
