/*
 * lpc17xx_trace/usb_msc/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/generic/mmcsd.h>
#include <halm/generic/sdio_spi.h>
#include <halm/pin.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/gptimer.h>
#include <halm/platform/lpc/serial.h>
#include <halm/platform/lpc/spi_dma.h>
#include <halm/platform/lpc/usb_device.h>
#include <halm/usb/msc.h>
#include <halm/usb/usb_trace.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_SIZE 8192
#define CS_PIN      PIN(0, 22)
#define LED_R       PIN(1, 9)
#define LED_W       PIN(1, 10)
#define SPI_CHANNEL 0
/*----------------------------------------------------------------------------*/
static const struct GpTimerConfig busyTimerConfig = {
    .frequency = 100000,
    .priority = 1,
    .channel = 0
};

static const struct GpTimerConfig chronoTimerConfig = {
    .frequency = 1000000,
    .priority = 2,
    .channel = 2
};

static const struct SerialConfig serialConfig = {
    .rate = 1500000,
    .rxLength = 16,
    .txLength = 8192,
    .rx = PIN(0, 3),
    .tx = PIN(0, 2),
    .priority = 1,
    .channel = 0
};

static const struct SpiDmaConfig spiConfig[] = {
    {
        .rate = 12000000,
        .sck = PIN(0, 15),
        .miso = PIN(0, 17),
        .mosi = PIN(0, 18),
        .dma = {0, 1},
        .channel = 0,
        .mode = 3
    }, {
        .rate = 12000000,
        .sck = PIN(0, 7),
        .miso = PIN(0, 8),
        .mosi = PIN(0, 9),
        .dma = {3, 2},
        .channel = 1,
        .mode = 3
    }
};

static const struct UsbDeviceConfig usbConfig = {
    .dm = PIN(0, 30),
    .dp = PIN(0, 29),
    .connect = PIN(2, 9),
    .vbus = PIN(1, 30),
    .vid = 0x15A2,
    .pid = 0x0044,
    .priority = 0,
    .channel = 0
};
/*----------------------------------------------------------------------------*/
static struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct PllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 4,
    .multiplier = 32
};

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_PLL
};
/*----------------------------------------------------------------------------*/
static uint8_t arena[BUFFER_SIZE];
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &mainClockConfig);

  clockEnable(UsbClock, &mainClockConfig);
  while (!clockReady(UsbClock));
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

  struct Interface * const serial = init(Serial, &serialConfig);
  assert(serial);

  /* Chrono timer */
  struct Timer * const chronoTimer = init(GpTimer, &chronoTimerConfig);
  assert(chronoTimer);
  timerEnable(chronoTimer);

  /* SDIO timer */
  struct Timer * const busyTimer = init(GpTimer, &busyTimerConfig);
  assert(busyTimer);
  timerSetOverflow(busyTimer, 20); /* 2 kHz event rate */

  /* Initialize SPI layer */
  struct Interface * const spi = init(SpiDma, &spiConfig[SPI_CHANNEL]);
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

  /* Initialize SD Card layer */
  const struct MMCSDConfig cardConfig = {
      .interface = sdio,
      .crc = false
  };
  struct Interface * const card = init(MMCSD, &cardConfig);
  assert(card);
  ifSetParam(card, IF_ZEROCOPY, 0);

  /* Initialize USB peripheral */
  struct Entity * const usb = init(UsbDevice, &usbConfig);
  assert(usb);

  /* Initialize Mass Storage Device */
  const struct MscConfig config = {
      .device = usb,
      .storage = card,

      .buffer = arena,
      .size = sizeof(arena),

      .endpoints = {
          .rx = 0x02,
          .tx = 0x82
      }
  };
  struct Msc * const msc = init(Msc, &config);
  assert(msc);
  (void)msc;

  usbTraceInit(serial, chronoTimer);
  usbDevSetConnected(usb, true);

  while (1);
  return 0;
}
