/*
 * lpc17xx_default/usb_msc/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/generic/sdcard.h>
#include <halm/generic/sdio_spi.h>
#include <halm/pin.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/lpc17xx/clocking.h>
#include <halm/platform/nxp/spi.h>
#include <halm/platform/nxp/spi_dma.h>
#include <halm/platform/nxp/usb_device.h>
#include <halm/usb/msc.h>
#include "interface_wrapper.h"
/*----------------------------------------------------------------------------*/
#define BUFFER_SIZE (4 * 512)
#define CS_PIN      PIN(0, 22)
#define LED_R       PIN(1, 9)
#define LED_W       PIN(1, 10)

#define TEST_DMA
#define TEST_INDICATION

#ifdef TEST_DMA
#define SPI_CLASS SpiDma
#else
#define SPI_CLASS Spi
#endif

#define SPI_CHANNEL 0
/*----------------------------------------------------------------------------*/
static const struct GpTimerConfig busyTimerConfig = {
    .frequency = 100000,
    .channel = 1
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

#ifdef TEST_DMA
static const struct SpiDmaConfig spiConfig[] = {
    {
        .rate = 8000000,
        .sck = PIN(0, 15),
        .miso = PIN(0, 17),
        .mosi = PIN(0, 18),
        .dma = {0, 1},
        .channel = 0,
        .mode = 3
    },
    {
        .rate = 8000000,
        .sck = PIN(0, 7),
        .miso = PIN(0, 8),
        .mosi = PIN(0, 9),
        .dma = {3, 2},
        .channel = 1,
        .mode = 3
    }
};
#else
static const struct SpiConfig spiConfig[] = {
    {
        .rate = 8000000,
        .sck = PIN(0, 15),
        .miso = PIN(0, 17),
        .mosi = PIN(0, 18),
        .channel = 0,
        .mode = 3
    },
    {
        .rate = 8000000,
        .sck = PIN(0, 7),
        .miso = PIN(0, 8),
        .mosi = PIN(0, 9),
        .channel = 1,
        .mode = 3
    }
};
#endif
/*----------------------------------------------------------------------------*/
static struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct PllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 4,
    .multiplier = 32
};

static const struct CommonClockConfig pllClockSource = {
    .source = CLOCK_PLL
};
/*----------------------------------------------------------------------------*/
static void setupClock()
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &pllClockSource);

  clockEnable(UsbClock, &pllClockSource);
  while (!clockReady(UsbClock));
}
/*----------------------------------------------------------------------------*/
static uint8_t transferBuffer[BUFFER_SIZE];
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

  /* Helper timer */
  struct Timer * const busyTimer = init(GpTimer, &busyTimerConfig);
  assert(busyTimer);
  timerSetOverflow(busyTimer, 50); /* 2 kHz event rate */

  /* Initialize SPI layer */
  struct Interface * const spi = init(SPI_CLASS, &spiConfig[SPI_CHANNEL]);
  assert(spi);

  /* Initialize SDIO layer */
  const struct SdioSpiConfig sdioConfig = {
      .interface = spi,
      .timer = busyTimer,
      .blocks = 0,
      .cs = CS_PIN
  };
  struct Interface * const sdio = init(SdioSpi, &sdioConfig);
  assert(sdio);

#ifdef TEST_INDICATION
  /* Optional wrapper for R/W operations indication */
  const struct InterfaceWrapperConfig wrapperConfig = {
      .pipe = sdio,
      .rx = LED_R,
      .tx = LED_W
  };
  struct Interface * const wrapper = init(InterfaceWrapper, &wrapperConfig);
  assert(wrapper);
#else
  struct Interface * const wrapper = sdio;
#endif

  /* Initialize SD Card layer */
  const struct SdCardConfig cardConfig = {
      .interface = wrapper,
      .crc = false
  };
  struct Interface * const card = init(SdCard, &cardConfig);
  assert(card);
  ifSetParam(card, IF_ZEROCOPY, 0);

  uint64_t cardSize;
  ifGetParam(card, IF_SIZE, &cardSize);

  /* Initialize USB peripheral */
  struct Entity * const usb = init(UsbDevice, &usbConfig);
  assert(usb);

  /* Initialize Mass Storage Device */
  const struct MscConfig config = {
      .device = usb,
      .storage = card,

      .buffer = transferBuffer,
      .size = sizeof(transferBuffer),

      .endpoints = {
          .rx = 0x02,
          .tx = 0x82
      }
  };
  struct Msc * const msc = init(Msc, &config);
  assert(msc);

  usbDevSetConnected(usb, true);

  while (1);

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
