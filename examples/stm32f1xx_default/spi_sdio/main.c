/*
 * stm32f1xx_default/spi_sdio/main.c
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/generic/mmcsd.h>
#include <halm/generic/sdio_spi.h>
#include <halm/generic/spi.h>
#include <xcore/memory.h>
#include <assert.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_SIZE     2048
#define SDIO_POLL_RATE  5000
/*----------------------------------------------------------------------------*/
static uint8_t arena[BUFFER_SIZE];
/*----------------------------------------------------------------------------*/
static void markBuffer(uint8_t *buffer, size_t size, uint32_t iteration)
{
  static const char begin[] = {'\x00', '\x11', '\x22', '\x33'};
  static const char end[] = {'\xCC', '\xDD', '\xEE', '\xFF'};

  uint8_t hex[sizeof(iteration) * 2];
  uint8_t *iterator = hex;

  for (int i = sizeof(iteration) * 2 - 1; i >= 0; --i)
  {
    const uint8_t nibble = iteration >> (i * 4);
    *iterator++ = nibble < 10 ? nibble + '0' : nibble + 'A' - 10;
  }

  memset(buffer + sizeof(begin) + sizeof(hex), 0,
      size - sizeof(begin) - sizeof(end) - sizeof(hex) * 2);
  memcpy(buffer, begin, sizeof(begin));
  memcpy(buffer + sizeof(begin), hex, sizeof(hex));
  memcpy(buffer + size - sizeof(hex) - sizeof(end), end, sizeof(end));
  memcpy(buffer + size - sizeof(hex), hex, sizeof(hex));
}
/*----------------------------------------------------------------------------*/
static void onDataEvent(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
static bool dataRead(struct Interface *card, uint8_t *buffer, size_t length,
    uint64_t position)
{
  bool event = false;

  ifSetParam(card, IF_POSITION, &position);
  ifSetCallback(card, onDataEvent, &event);

  const size_t count = ifRead(card, buffer, length);
  bool completed = false;

  if (count == length)
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
static bool dataWrite(struct Interface *card, uint8_t *buffer, size_t length,
    uint64_t position)
{
  bool event = false;

  ifSetParam(card, IF_POSITION, &position);
  ifSetCallback(card, onDataEvent, &event);
  markBuffer(buffer, length, position / length);

  const size_t count = ifWrite(card, buffer, length);
  bool completed = false;

  if (count == length)
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
  static const uint32_t SPI_SDIO_RATE = 12000000;
  static const uint8_t SPI_SDIO_MODE = 3;
  static const bool ENABLE_WRITE_TEST = false;
  static const bool USE_BUSY_TIMER = true;

  uint64_t capacity;
  uint64_t position = 0;
  struct Interface *card;
  struct Interface *sdio;
  struct Interface *spi;
  enum Result res;
  bool event = false;

  boardSetupClockPll();

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, BOARD_LED_INV);

  /* Helper timer for SDIO status polling */
  struct Timer * const busyTimer = USE_BUSY_TIMER ? boardSetupAdcTimer() : 0;

  if (busyTimer)
  {
    /* Set 5 kHz update event rate */
    assert(timerGetFrequency(busyTimer) >= 10 * SDIO_POLL_RATE);
    timerSetOverflow(busyTimer, timerGetFrequency(busyTimer) / SDIO_POLL_RATE);
  }

  /* Initialize SPI layer */
  spi = boardSetupSpi();
  res = ifSetParam(spi, IF_RATE, &SPI_SDIO_RATE);
  assert(res == E_OK);
  res = ifSetParam(spi, IF_SPI_MODE, &SPI_SDIO_MODE);
  assert(res == E_OK);

  /* Initialize SDIO layer */
  const struct SdioSpiConfig sdioConfig = {
      .interface = spi,
      .timer = busyTimer,
      .wq = 0,
      .blocks = 0,
      .cs = BOARD_SDIO_CS
  };
  sdio = init(SdioSpi, &sdioConfig);
  assert(sdio);

  /* Initialize SD Card layer */
  const struct MMCSDConfig cardConfig = {
      .interface = sdio,
      .crc = false
  };
  card = init(MMCSD, &cardConfig);
  assert(card);
  res = ifSetParam(card, IF_ZEROCOPY, 0);
  assert(res == E_OK);
  res = ifGetParam(card, IF_SIZE_64, &capacity);
  assert(res == E_OK);

  /* Configure the timer for read/write events */
  struct Timer * const eventTimer = boardSetupTimer();
  timerSetOverflow(eventTimer, timerGetFrequency(eventTimer));
  timerSetCallback(eventTimer, onTimerOverflow, &event);
  timerEnable(eventTimer);

  /* Suppress warning */
  (void)res;

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    if (ENABLE_WRITE_TEST)
    {
      pinWrite(led, !BOARD_LED_INV);
      if (dataWrite(card, arena, sizeof(arena), position))
        pinWrite(led, BOARD_LED_INV);
    }

    pinWrite(led, !BOARD_LED_INV);
    if (dataRead(card, arena, sizeof(arena), position))
      pinWrite(led, BOARD_LED_INV);

    /* Increment position */
    position += sizeof(arena);
    if (position >= capacity)
      position = 0;
  }

  return 0;
}
