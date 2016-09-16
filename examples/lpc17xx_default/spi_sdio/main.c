/*
 * main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include <halm/pin.h>
#include <halm/platform/sdcard.h>
#include <halm/platform/sdio_spi.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/spi.h>
#include <halm/platform/nxp/spi_dma.h>
#include <halm/platform/nxp/lpc17xx/clocking.h>
/*----------------------------------------------------------------------------*/
#define BLOCK_SIZE 512
#define LED_PIN PIN(0, 22)

#define TEST_CRC
#define TEST_DMA
#define TEST_WATCHDOG
#define TEST_WRITE

#ifdef TEST_CRC
#define SDIO_CRC true
#else
#define SDIO_CRC false
#endif

#ifdef TEST_DMA
#define SPI_CLASS SpiDma
#else
#define SPI_CLASS Spi
#endif

#define SPI_CHANNEL 0
/*----------------------------------------------------------------------------*/
static const struct GpTimerConfig timerConfig = {
    .frequency = 1000000,
    .channel = 0
};

#ifdef TEST_WATCHDOG
static const struct GpTimerConfig watchdogConfig = {
    .frequency = 100000,
    .channel = 1
};
#endif

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

static struct PllConfig sysPllConfig = {
    .multiplier = 24,
    .divisor = 3,
    .source = CLOCK_EXTERNAL
};

static struct CommonClockConfig mainClkConfig = {
    .source = CLOCK_PLL
};
/*----------------------------------------------------------------------------*/
#ifdef TEST_WRITE
static char binToHex(uint8_t value)
{
  const uint8_t nibble = value & 0x0F;

  return nibble < 10 ? nibble + '0' : nibble + 'A' - 10;
}
#endif
/*----------------------------------------------------------------------------*/
#ifdef TEST_WRITE
static void numberToHex(uint8_t *output, uint32_t value)
{
  for (int i = sizeof(value) * 2 - 1; i >= 0; --i)
    *output++ = binToHex((uint8_t)(value >> 4 * i));
}
#endif
/*----------------------------------------------------------------------------*/
#ifdef TEST_WRITE
static void markBuffer(uint8_t *buffer, size_t size, uint32_t iteration)
{
  static const char begin[] = {'\x00', '\x11', '\x22', '\x33'};
  static const char end[] = {'\xCC', '\xDD', '\xEE', '\xFF'};

  uint8_t hex[sizeof(iteration) * 2];
  numberToHex(hex, iteration);

  memset(buffer + sizeof(begin) + sizeof(hex), 0,
      size - sizeof(begin) - sizeof(end) - sizeof(hex) * 2);
  memcpy(buffer, begin, sizeof(begin));
  memcpy(buffer + sizeof(begin), hex, sizeof(hex));
  memcpy(buffer + size - sizeof(hex) - sizeof(end), end, sizeof(end));
  memcpy(buffer + size - sizeof(hex), hex, sizeof(hex));
}
#endif
/*----------------------------------------------------------------------------*/
static void setupClock()
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &mainClkConfig);
}
/*----------------------------------------------------------------------------*/
static void onEvent(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
#ifdef TEST_WRITE
static bool dataWrite(struct Interface *card, uint8_t *buffer, size_t size,
    uint64_t position)
{
  bool event;

  markBuffer(buffer, size, position / size);

  ifSet(card, IF_ADDRESS, &position);
  ifCallback(card, onEvent, &event);

  const size_t bytesWritten = ifWrite(card, buffer, size);
  bool completed = false;

  if (bytesWritten == size)
  {
    while (!event)
      barrier();
    event = false;

    if (ifGet(card, IF_STATUS, 0) == E_OK)
      completed = true;
  }

  ifCallback(card, 0, 0);
  return completed;
}
#endif
/*----------------------------------------------------------------------------*/
static bool dataRead(struct Interface *card, uint8_t *buffer, size_t size,
    uint64_t position)
{
  bool event;

  ifSet(card, IF_ADDRESS, &position);
  ifCallback(card, onEvent, &event);

  const size_t bytesRead = ifRead(card, buffer, size);
  bool completed = false;

  if (bytesRead == size)
  {
    while (!event)
      barrier();
    event = false;

    if (ifGet(card, IF_STATUS, 0) == E_OK)
      completed = true;
  }

  ifCallback(card, 0, 0);
  return completed;
}
/*----------------------------------------------------------------------------*/
static uint8_t transferBuffer[BLOCK_SIZE * 2];
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

#ifdef TEST_WATCHDOG
  struct Timer * const watchdog = init(GpTimer, &watchdogConfig);
  assert(watchdog);
#else
  struct Timer * const watchdog = 0;
#endif

  /* Initialize SPI layer */
  struct Interface * const spi = init(SPI_CLASS, &spiConfig[SPI_CHANNEL]);
  assert(spi);

  /* Initialize SDIO layer */
  const struct SdioSpiConfig sdioConfig = {
      .interface = spi,
      .timer = watchdog,
      .blocks = sizeof(transferBuffer) / BLOCK_SIZE,
      .cs = PIN(0, 16)
  };
  struct Interface * const sdio = init(SdioSpi, &sdioConfig);
  assert(sdio);

  /* Initialize SD Card layer */
  const struct SdCardConfig cardConfig = {
      .interface = sdio,
      .crc = SDIO_CRC
  };
  struct Interface * const card = init(SdCard, &cardConfig);
  assert(card);
  ifSet(card, IF_ZEROCOPY, 0);

  uint64_t cardSize;
  ifGet(card, IF_SIZE, &cardSize);

  /* Configure LED and variables for storing current state */
  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, 0);

  uint64_t position = 0;
  bool event = false;

  /* Configure the timer for read/write events */
  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  timerSetOverflow(timer, 1000000);
  timerCallback(timer, onEvent, &event);

  timerSetEnabled(timer, true);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

#ifdef TEST_WRITE
    pinSet(led);
    if (dataWrite(card, transferBuffer, sizeof(transferBuffer), position))
      pinReset(led);
#endif

    pinSet(led);
    if (dataRead(card, transferBuffer, sizeof(transferBuffer), position))
      pinReset(led);

    /* Increment position */
    position += sizeof(transferBuffer);
    if (position >= cardSize)
      position = 0;
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
