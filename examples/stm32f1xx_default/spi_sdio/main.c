/*
 * stm32f1xx_default/spi_sdio/main.c
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/generic/mmcsd.h>
#include <halm/generic/sdio_spi.h>
#include <halm/pin.h>
#include <halm/platform/stm32/clocking.h>
#include <halm/platform/stm32/gptimer.h>
#include <halm/platform/stm32/spi.h>
#include <assert.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_SIZE 2048
#define CS_PIN      PIN(PORT_A, 4)
#define LED_PIN     PIN(PORT_C, 13)

#define TEST_BUSY_TIMER
/* #define TEST_WRITE */

#define SPI_CHANNEL 0
/*----------------------------------------------------------------------------*/
static const struct GpTimerConfig eventTimerConfig = {
    .frequency = 10000,
    .channel = TIM2
};

#ifdef TEST_BUSY_TIMER
static const struct GpTimerConfig busyTimerConfig = {
    .frequency = 100000,
    .channel = TIM3
};
#endif

static const struct SpiConfig spiConfig[] = {
    {
        .rate = 4500000,
        .miso = PIN(PORT_A, 6),
        .mosi = PIN(PORT_A, 7),
        .sck = PIN(PORT_A, 5),
        .channel = 0,
        .mode = 0,
        .rxDma = DMA1_STREAM2,
        .txDma = DMA1_STREAM3
    },
    {
        .rate = 4500000,
        .miso = PIN(PORT_B, 14),
        .mosi = PIN(PORT_B, 15),
        .sck = PIN(PORT_B, 13),
        .channel = 1,
        .mode = 0,
        .rxDma = DMA1_STREAM4,
        .txDma = DMA1_STREAM5
    }
};
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 8000000
};

static const struct MainPllConfig mainPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 1,
    .multiplier = 9
};

static const struct SystemClockConfig systemClockConfig = {
    .source = CLOCK_PLL
};

static const struct BusClockConfig ahbBusClockConfig = {
    .divisor = 1
};

static const struct BusClockConfig apbBusClockConfig = {
    .divisor = 2
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
static void setupClock(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainPll, &mainPllConfig);
  while (!clockReady(MainPll));

  clockEnable(Apb1Clock, &apbBusClockConfig);
  clockEnable(Apb2Clock, &apbBusClockConfig);
  clockEnable(SystemClock, &systemClockConfig);

  clockEnable(MainClock, &ahbBusClockConfig);
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

  ifSetParam(card, IF_POSITION, &position);
  ifSetCallback(card, onEvent, &event);

  const size_t bytesWritten = ifWrite(card, buffer, size);
  bool completed = false;

  if (bytesWritten == size)
  {
    while (!event)
      barrier();
    event = false;

    if (ifGetParam(card, IF_STATUS, 0) == E_OK)
      completed = true;
  }

  ifSetCallback(card, 0, 0);
  return completed;
}
#endif
/*----------------------------------------------------------------------------*/
static bool dataRead(struct Interface *card, uint8_t *buffer, size_t size,
    uint64_t position)
{
  bool event;

  ifSetParam(card, IF_POSITION, &position);
  ifSetCallback(card, onEvent, &event);

  const size_t bytesRead = ifRead(card, buffer, size);
  bool completed = false;

  if (bytesRead == size)
  {
    while (!event)
      barrier();
    event = false;

    if (ifGetParam(card, IF_STATUS, 0) == E_OK)
      completed = true;
  }

  ifSetCallback(card, 0, 0);
  return completed;
}
/*----------------------------------------------------------------------------*/
static uint8_t arena[BUFFER_SIZE];
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

#ifdef TEST_BUSY_TIMER
  struct Timer * const busyTimer = init(GpTimer, &busyTimerConfig);
  assert(busyTimer);
  /* Set 2 kHz event rate at 100 kHz timer frequency */
  timerSetOverflow(busyTimer, 50);
#else
  struct Timer * const busyTimer = 0;
#endif

  /* Initialize SPI layer */
  struct Interface * const spi = init(Spi, &spiConfig[SPI_CHANNEL]);
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

  uint64_t cardSize;
  ifGetParam(card, IF_SIZE_64, &cardSize);

  /* Configure LED and variables for storing current state */
  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  uint64_t position = 0;
  bool event = false;

  /* Configure the timer for read/write events */
  struct Timer * const eventTimer = init(GpTimer, &eventTimerConfig);
  assert(eventTimer);
  timerSetOverflow(eventTimer, timerGetFrequency(eventTimer));
  timerSetCallback(eventTimer, onEvent, &event);

  timerEnable(eventTimer);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

#ifdef TEST_WRITE
    pinSet(led);
    if (dataWrite(card, arena, sizeof(arena), position))
      pinReset(led);
#endif

    pinSet(led);
    if (dataRead(card, arena, sizeof(arena), position))
      pinReset(led);

    /* Increment position */
    position += sizeof(arena);
    if (position >= cardSize)
      position = 0;
  }

  return 0;
}
