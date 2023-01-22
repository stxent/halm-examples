/*
 * lpc17xx_default/shared/board.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/lpc/adc.h>
#include <halm/platform/lpc/adc_oneshot.h>
#include <halm/platform/lpc/adc_dma.h>
#include <halm/platform/lpc/adc_dma_stream.h>
#include <halm/platform/lpc/can.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/dac.h>
#include <halm/platform/lpc/dac_dma.h>
#include <halm/platform/lpc/gptimer.h>
#include <halm/platform/lpc/i2c.h>
#include <halm/platform/lpc/i2s_dma.h>
#include <halm/platform/lpc/serial.h>
#include <halm/platform/lpc/serial_dma.h>
#include <halm/platform/lpc/spi.h>
#include <halm/platform/lpc/spi_dma.h>
#include <halm/platform/lpc/usb_device.h>
#include <halm/platform/lpc/wdt.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
const PinNumber adcPinArray[] = {
    PIN(0, 25),
    PIN(1, 31),
    PIN(0, 3),
    PIN(0, 2),
    0
};

static const struct AdcConfig adcConfig = {
    .pins = adcPinArray,
    .event = ADC_TIMER1_MAT1,
    .channel = 0
};

static const struct AdcDmaConfig adcDmaConfig = {
    .pins = adcPinArray,
    .event = ADC_TIMER1_MAT1,
    .channel = 0,
    .dma = 4
};

static const struct AdcDmaStreamConfig adcStreamConfig = {
    .pins = adcPinArray,
    .size = 2,
    .converter = {ADC_TIMER1_MAT1, 0},
    .memory = {GPDMA_MAT0_0, 1},
    .channel = 0
};

static const struct AdcOneShotConfig adcOneShotConfig = {
    .pin = PIN(0, 25),
    .channel = 0
};

static const struct GpTimerConfig adcTimerConfig = {
    .frequency = 1000000,
    .event = GPTIMER_MATCH1,
    .channel = 1
};

static const struct CanConfig canConfig = {
    .timer = 0,
    .rate = 1000000,
    .rxBuffers = 4,
    .txBuffers = 4,
    .rx = PIN(0, 0),
    .tx = PIN(0, 1),
    .priority = 0,
    .channel = 0
};

static const struct DacConfig dacConfig = {
    .pin = PIN(0, 26),
    .value = 32768
};

static const struct DacDmaConfig dacDmaConfig = {
    .size = 2,
    .rate = 96000,
    .value = 32768,
    .pin = PIN(0, 26),
    .dma = 5
};

static const struct I2CConfig i2cConfig = {
    .rate = 100000,
    .scl = PIN(0, 11),
    .sda = PIN(0, 10),
    .channel = 2
};

static const struct I2SDmaConfig i2sConfig = {
    .size = 2,
    .rate = 44100,
    .width = I2S_WIDTH_16,
    .tx = {
        .sda = PIN(0, 9),
        .sck = PIN(0, 7),
        .ws = PIN(0, 8),
        .mclk = PIN(4, 29),
        .dma = 6
    },
    .rx = {
        .sda = PIN(0, 6),
        .dma = 7
    },
    .channel = 0,
    .mono = false,
    .slave = false
};

static const struct SerialConfig serialConfig = {
    .rxLength = BOARD_UART_BUFFER,
    .txLength = BOARD_UART_BUFFER,
    .rate = 19200,
    .rx = PIN(0, 16),
    .tx = PIN(0, 15),
    .channel = 1
};

static const struct SerialDmaConfig serialDmaConfig = {
    .rxChunks = 4,
    .rxLength = BOARD_UART_BUFFER,
    .txLength = BOARD_UART_BUFFER,
    .rate = 19200,
    .rx = PIN(0, 16),
    .tx = PIN(0, 15),
    .channel = 1,
    .dma = {2, 3}
};

static const struct SpiConfig spiConfig = {
    .rate = 2000000,
    .miso = PIN(0, 17),
    .mosi = PIN(0, 18),
    .sck = PIN(1, 20),
    .priority = 0,
    .channel = 0,
    .mode = 3
};

static const struct SpiDmaConfig spiDmaConfig = {
    .rate = 2000000,
    .miso = PIN(0, 17),
    .mosi = PIN(0, 18),
    .sck = PIN(1, 20),
    .channel = 0,
    .mode = 3,
    .dma = {0, 1}
};

static const struct GpTimerConfig timerConfig = {
    .frequency = 1000000,
    .event = GPTIMER_MATCH0,
    .channel = 0
};

static const struct UsbDeviceConfig usbConfig = {
    .dm = PIN(0, 30),
    .dp = PIN(0, 29),
    .connect = PIN(2, 9),
    .vbus = PIN(1, 30),
    .vid = 0x15A2,
    .pid = 0x0044,
    .channel = 0
};
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct PllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 4,
    .multiplier = 32
};

static const struct PllConfig usbPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 4,
    .multiplier = 16
};

static const struct GenericClockConfig mainClockConfigExt = {
    .source = CLOCK_EXTERNAL
};

static const struct GenericClockConfig mainClockConfigPll = {
    .source = CLOCK_PLL
};

static const struct GenericClockConfig usbClockConfig = {
    .source = CLOCK_USB_PLL
};

static const struct WdtConfig wdtConfig = {
    .period = 1000,
    .source = WDT_CLOCK_PCLK
};
/*----------------------------------------------------------------------------*/
size_t boardGetAdcPinCount(void)
{
  return ARRAY_SIZE(adcPinArray) - 1;
}
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainClock, &mainClockConfigExt);
}
/*----------------------------------------------------------------------------*/
void boardSetupClockPll(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(UsbPll, &usbPllConfig);
  while (!clockReady(UsbPll));

  clockEnable(MainClock, &mainClockConfigPll);

  clockEnable(UsbClock, &usbClockConfig);
  while (!clockReady(UsbClock));
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupAdc(void)
{
  struct Interface * const interface = init(Adc, &adcConfig);
  assert(interface);
  return(interface);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupAdcDma(void)
{
  struct Interface * const interface = init(AdcDma, &adcDmaConfig);
  assert(interface);
  return(interface);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupAdcOneShot(void)
{
  struct Interface * const interface = init(AdcOneShot, &adcOneShotConfig);
  assert(interface);
  return(interface);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupAdcStream(void)
{
  struct Interface * const interface = init(AdcDmaStream, &adcStreamConfig);
  assert(interface);
  return(interface);
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupAdcTimer(void)
{
  struct Timer * const timer = init(GpTimer, &adcTimerConfig);
  assert(timer);
  return(timer);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupCan(struct Timer *timer)
{
  /* Override default config */
  struct CanConfig config = canConfig;
  config.timer = timer;

  struct Interface * const interface = init(Can, &config);
  assert(interface);
  return(interface);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupDac(void)
{
  struct Interface * const interface = init(Dac, &dacConfig);
  assert(interface);
  return(interface);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupDacDma(void)
{
  struct Interface * const interface = init(DacDma, &dacDmaConfig);
  assert(interface);
  return(interface);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupI2C(void)
{
  struct Interface * const interface = init(I2C, &i2cConfig);
  assert(interface);
  return(interface);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupI2S(void)
{
  struct Interface * const interface = init(I2SDma, &i2sConfig);
  assert(interface);
  return(interface);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerial(void)
{
  struct Interface * const interface = init(Serial, &serialConfig);
  assert(interface);
  return(interface);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerialDma(void)
{
  struct Interface * const interface = init(SerialDma, &serialDmaConfig);
  assert(interface);
  return(interface);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpi(void)
{
  struct Interface * const interface = init(Spi, &spiConfig);
  assert(interface);
  return(interface);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpiDma(void)
{
  struct Interface * const interface = init(SpiDma, &spiDmaConfig);
  assert(interface);
  return(interface);
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimer(void)
{
  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  return(timer);
}
/*----------------------------------------------------------------------------*/
struct Entity *boardSetupUsb(void)
{
  struct Entity * const usb = init(UsbDevice, &usbConfig);
  assert(usb);
  return(usb);
}
/*----------------------------------------------------------------------------*/
struct Watchdog *boardSetupWdt(void)
{
  struct Watchdog * const timer = init(Wdt, &wdtConfig);
  assert(timer);
  return(timer);
}
