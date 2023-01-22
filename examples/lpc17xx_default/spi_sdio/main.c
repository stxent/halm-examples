/*
 * lpc17xx_default/spi_sdio/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <board.h>
#include <halm/generic/mmcsd.h>
#include <halm/generic/sdio_spi.h>
#include <halm/platform/lpc/gptimer.h>
#include <assert.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_SIZE 2048

#define TEST_BUSY_TIMER
#define TEST_DMA
/* #define TEST_WRITE */
/*----------------------------------------------------------------------------*/
#ifdef TEST_BUSY_TIMER
static const struct GpTimerConfig busyTimerConfig = {
    .frequency = 1000000,
    .channel = 1
};
#endif
/*----------------------------------------------------------------------------*/
static uint8_t arena[BUFFER_SIZE];
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
static void onTimerOverflow(void *argument)
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
  ifSetCallback(card, onTimerOverflow, &event);

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
int main(void)
{
  static const uint32_t spiSdioRate = 12000000;
  struct Timer *busyTimer = 0;

  boardSetupClockPll();

#ifdef TEST_BUSY_TIMER
  busyTimer = init(GpTimer, &busyTimerConfig);
  assert(busyTimer);
  /* Set 2 kHz event rate */
  timerSetOverflow(busyTimer, timerGetFrequency(busyTimer) / 2000);
#endif

  /* Initialize SPI layer */
#ifdef TEST_DMA
  struct Interface * const spi = boardSetupSpiDma();
#else
  struct Interface * const spi = boardSetupSpi();
#endif
  ifSetParam(spi, IF_RATE, &spiSdioRate);

  /* Initialize SDIO layer */
  const struct SdioSpiConfig sdioConfig = {
      .interface = spi,
      .timer = busyTimer,
      .wq = 0,
      .blocks = 0,
      .cs = BOARD_SDIO_CS
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
  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, false);

  uint64_t position = 0;
  bool event = false;

  /* Configure the timer for read/write events */
  struct Timer * const eventTimer = boardSetupTimer();
  timerSetOverflow(eventTimer, timerGetFrequency(eventTimer));
  timerSetCallback(eventTimer, onTimerOverflow, &event);

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
