/*
 * main.c
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
/*----------------------------------------------------------------------------*/
#define BLOCK_SIZE  512
#define LED_PIN     PIN(0, 22)

#define TEST_DMA

#ifdef TEST_DMA
#define SPI_CLASS SpiDma
#else
#define SPI_CLASS Spi
#endif

#define SPI_CHANNEL 0
/*----------------------------------------------------------------------------*/
static const struct GpTimerConfig sdioTimerConfig = {
    .frequency = 100000,
    .channel = 1
};

static const struct UsbDeviceConfig usbConfig = {
    .dm = PIN(0, 30),
    .dp = PIN(0, 29),
    .connect = PIN(2, 9),
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
        .channel = 0
    },
    {
        .rate = 8000000,
        .sck = PIN(0, 7),
        .miso = PIN(0, 8),
        .mosi = PIN(0, 9),
        .dma = {3, 2},
        .channel = 1
    }
};
#else
static const struct SpiConfig spiConfig[] = {
    {
        .rate = 8000000,
        .sck = PIN(0, 15),
        .miso = PIN(0, 17),
        .mosi = PIN(0, 18),
        .channel = 0
    },
    {
        .rate = 8000000,
        .sck = PIN(0, 7),
        .miso = PIN(0, 8),
        .mosi = PIN(0, 9),
        .channel = 1
    }
};
#endif
/*----------------------------------------------------------------------------*/
static struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct PllConfig sysPllConfig = {
    .multiplier = 32,
    .divisor = 4,
    .source = CLOCK_EXTERNAL
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
static uint8_t transferBuffer[BLOCK_SIZE * 2];
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

  /* Configure LED and variables for storing current state */
  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, 0);

  /* Helper timer */
  struct Timer * const sdioTimer = init(GpTimer, &sdioTimerConfig);
  assert(sdioTimer);

  /* Initialize SPI layer */
  struct Interface * const spi = init(SPI_CLASS, &spiConfig[SPI_CHANNEL]);
  assert(spi);

  /* Initialize SDIO layer */
  const struct SdioSpiConfig sdioConfig = {
      .interface = spi,
      .timer = sdioTimer,
      .blocks = 0,
      .cs = PIN(0, 16)
  };
  struct Interface * const sdio = init(SdioSpi, &sdioConfig);
  assert(sdio);

  /* Initialize SD Card layer */
  const struct SdCardConfig cardConfig = {
      .interface = sdio,
      .crc = false
  };
  struct Interface * const card = init(SdCard, &cardConfig);
  assert(card);
  ifSet(card, IF_ZEROCOPY, 0);

  uint64_t cardSize;
  ifGet(card, IF_SIZE, &cardSize);

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

  bool event = false;

  while (1)
  {
    while (!event)
      barrier();
    event = false;
  }

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
