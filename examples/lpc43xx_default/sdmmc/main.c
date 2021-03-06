/*
 * lpc43xx_default/sdmmc/main.c
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/generic/mmcsd.h>
#include <halm/pin.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/gptimer.h>
#include <halm/platform/lpc/sdmmc.h>
#include <assert.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_SIZE 2048
#define LED_PIN     PIN(PORT_6, 9)

/* #define TEST_1BIT */
/* #define TEST_WRITE */
/*----------------------------------------------------------------------------*/
static const struct GpTimerConfig eventTimerConfig = {
    .frequency = 1000000,
    .channel = 0
};

static const struct SdmmcConfig sdioConfig = {
    .rate = 12000000,
    .clk = PIN(PORT_CLK, 0),
    .cmd = PIN(PORT_1, 6),
#ifndef TEST_1BIT
    .dat0 = PIN(PORT_1, 9),
    .dat1 = PIN(PORT_1, 10),
    .dat2 = PIN(PORT_1, 11),
    .dat3 = PIN(PORT_1, 12)
#else
    .dat0 = PIN(PORT_1, 9)
#endif
};
/*----------------------------------------------------------------------------*/
static const struct GenericClockConfig initialClockConfig = {
    .source = CLOCK_INTERNAL
};

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_PLL
};

static const struct GenericClockConfig sdClockConfig = {
    .source = CLOCK_IDIVB
};

static const struct GenericDividerConfig dividerConfig = {
    .source = CLOCK_PLL,
    .divisor = 2
};

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000,
    .bypass = false
};

static const struct PllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 3,
    .multiplier = 24
};
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  *(bool *)argument = true;
}
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
static bool dataWrite(struct Interface *card, uint8_t *buffer, size_t size,
    uint64_t position)
{
  bool event;

  markBuffer(buffer, size, position / size);

  ifSetParam(card, IF_POSITION, &position);
  ifSetCallback(card, onTimerOverflow, &event);

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
#ifdef TEST_WRITE
static void numberToHex(uint8_t *output, uint32_t value)
{
  for (int i = sizeof(value) * 2 - 1; i >= 0; --i)
    *output++ = binToHex((uint8_t)(value >> 4 * i));
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
static void setupClock(void)
{
  clockEnable(MainClock, &initialClockConfig);

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(DividerB, &dividerConfig);
  while (!clockReady(DividerB));

  clockEnable(SdioClock, &sdClockConfig);
  while (!clockReady(SdioClock));

  clockEnable(MainClock, &mainClockConfig);
}
/*----------------------------------------------------------------------------*/
static uint8_t arena[BUFFER_SIZE];
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

  /* Initialize SDIO layer */
  struct Interface * const sdio = init(Sdmmc, &sdioConfig);
  assert(sdio);

  /* Initialize SD Card layer */
  const struct MMCSDConfig cardConfig = {
      .interface = sdio,
      .crc = true
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
  timerSetOverflow(eventTimer, 1000000);
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
