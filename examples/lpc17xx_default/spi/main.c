/*
 * main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/pin.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/spi.h>
#include <halm/platform/nxp/spi_dma.h>
#include <xcore/memory.h>
/*----------------------------------------------------------------------------*/
#define CS_PIN  PIN(0, 6)
#define LED_PIN PIN(0, 22)

#define TEST_DMA
#define TEST_ZEROCOPY

#ifdef TEST_DMA
#define SPI_CLASS SpiDma
#else
#define SPI_CLASS Spi
#endif

#define SPI_CHANNEL 1
/*----------------------------------------------------------------------------*/
static const struct GpTimerConfig timerConfig = {
    .frequency = 1000,
    .channel = 0
};
/*----------------------------------------------------------------------------*/
#ifdef TEST_DMA
static const struct SpiDmaConfig spiConfig[] = {
    {
        .rate = 400000,
        .sck = PIN(0, 15),
        .miso = PIN(0, 17),
        .mosi = PIN(0, 18),
        .dma = {0, 1},
        .channel = 0,
        .mode = 0
    },
    {
        .rate = 400000,
        .sck = PIN(0, 7),
        .miso = PIN(0, 8),
        .mosi = PIN(0, 9),
        .dma = {3, 2},
        .channel = 1,
        .mode = 0
    }
};
#else
static const struct SpiConfig spiConfig[] = {
    {
        .rate = 400000,
        .sck = PIN(0, 15),
        .miso = PIN(0, 17),
        .mosi = PIN(0, 18),
        .channel = 0,
        .mode = 0
    },
    {
        .rate = 400000,
        .sck = PIN(0, 7),
        .miso = PIN(0, 8),
        .mosi = PIN(0, 9),
        .channel = 1,
        .mode = 0
    }
};
#endif
/*----------------------------------------------------------------------------*/
#ifdef TEST_ZEROCOPY
static void onTransferCompleted(void *argument)
{
  ++(*(unsigned int *)argument);
}
#endif
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  const struct Pin cs = pinInit(CS_PIN);
  pinOutput(cs, true);

  struct Interface * const spi = init(SPI_CLASS, &spiConfig[SPI_CHANNEL]);
  assert(spi);

  static const uint32_t desiredRate = 200000;
  enum result res;

  res = ifSet(spi, IF_RATE, &desiredRate);
  assert(res == E_OK);

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  timerSetOverflow(timer, 250);

  unsigned int value = 0;
  bool event = false;

#ifdef TEST_ZEROCOPY
  res = ifSet(spi, IF_ZEROCOPY, 0);
  assert(res == E_OK);
  res = ifCallback(spi, onTransferCompleted, &value);
  assert(res == E_OK);
#endif

  (void)res; /* Suppress warning */

  timerCallback(timer, onTimerOverflow, &event);
  timerSetEnabled(timer, true);
  pinReset(cs);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    const uint32_t buffer = toBigEndian32(value);

    pinSet(led);
    ifWrite(spi, &buffer, sizeof(buffer));
    pinReset(led);

#ifndef TEST_ZEROCOPY
    ++value;
#endif
  }

  return 0;
}
