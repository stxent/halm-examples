/*
 * main.c
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <string.h>

#include <halm/generic/sdcard.h>
#include <halm/pin.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/lpc43xx/clocking.h>
#include <halm/platform/nxp/sdmmc.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_SIZE (4 * 512)
#define LED_PIN     PIN(PORT_6, 6)

/* #define TEST_1BIT */
#define TEST_WRITE
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
static const struct CommonClockConfig cardClock = {
    .source = CLOCK_IDIVB
};

static const struct CommonDividerConfig dividerConfig = {
    .value = 2,
    .source = CLOCK_PLL
};

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000,
    .bypass = false
};

static const struct PllConfig sysPllConfig = {
    .multiplier = 24,
    .divisor = 3,
    .source = CLOCK_EXTERNAL
};

static const struct CommonClockConfig mainClkConfig = {
    .source = CLOCK_PLL
};

static const struct CommonClockConfig initialClock = {
    .source = CLOCK_INTERNAL
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
  clockEnable(MainClock, &initialClock);

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(DividerB, &dividerConfig);
  while (!clockReady(DividerB));

  clockEnable(SdioClock, &cardClock);
  while (!clockReady(SdioClock));

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

  ifSet(card, IF_POSITION, &position);
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

  ifSet(card, IF_POSITION, &position);
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
static uint8_t transferBuffer[BUFFER_SIZE];
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

  /* Initialize SDIO layer */
  struct Interface * const sdio = init(Sdmmc, &sdioConfig);
  assert(sdio);

  /* Initialize SD Card layer */
  const struct SdCardConfig cardConfig = {
      .interface = sdio,
      .crc = true
  };
  struct Interface * const card = init(SdCard, &cardConfig);
  assert(card);
  ifSet(card, IF_ZEROCOPY, 0);

  uint64_t cardSize;
  ifGet(card, IF_SIZE, &cardSize);

  /* Configure LED and variables for storing current state */
  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  uint64_t position = 0;
  bool event = false;

  /* Configure the timer for read/write events */
  struct Timer * const eventTimer = init(GpTimer, &eventTimerConfig);
  assert(eventTimer);
  timerSetOverflow(eventTimer, 1000000);
  timerCallback(eventTimer, onEvent, &event);

  timerSetEnabled(eventTimer, true);

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
