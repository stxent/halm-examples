/*
 * stm32f1xx_default/spi/main.c
 * Copyright (C) 2018 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/core/cortex/systick.h>
#include <halm/platform/stm32/spi.h>
#include <xcore/memory.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define CS_PIN      PIN(PORT_A, 4)
#define LED_PIN     PIN(PORT_C, 9)
#define SPI_CHANNEL 0

#define TEST_ZEROCOPY
/*----------------------------------------------------------------------------*/
static const struct SpiConfig spiConfig[] = {
    {
        .rate = 2000000,
        .miso = PIN(PORT_A, 6),
        .mosi = PIN(PORT_A, 7),
        .sck = PIN(PORT_A, 5),
        .channel = 0,
        .mode = 0,
        .rxDma = DMA1_STREAM2,
        .txDma = DMA1_STREAM3
    }, {
        .rate = 2000000,
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

  struct Interface * const spi = init(Spi, &spiConfig[SPI_CHANNEL]);
  assert(spi);

  static const uint32_t desiredRate = 1000000;
  enum Result res;

  res = ifSetParam(spi, IF_RATE, &desiredRate);
  assert(res == E_OK);

  struct Timer * const timer = init(SysTickTimer, 0);
  assert(timer);
  timerSetOverflow(timer, timerGetFrequency(timer) / 4);

  unsigned int value = 0;
  bool event = false;

#ifdef TEST_ZEROCOPY
  res = ifSetParam(spi, IF_ZEROCOPY, 0);
  assert(res == E_OK);
  ifSetCallback(spi, onTransferCompleted, &value);
#endif

  (void)res; /* Suppress warning */

  timerSetCallback(timer, onTimerOverflow, &event);
  timerEnable(timer);
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
