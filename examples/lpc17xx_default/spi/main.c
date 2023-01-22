/*
 * lpc17xx_default/spi/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/timer.h>
#include <xcore/interface.h>
#include <xcore/memory.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define TEST_DMA
#define TEST_ZEROCOPY
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
  static const uint32_t spiTestRate = 400000;
  enum Result res;

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, false);

  const struct Pin cs = pinInit(BOARD_SPI_CS);
  pinOutput(cs, true);

#ifdef TEST_DMA
  struct Interface * const spi = boardSetupSpiDma();
#else
  struct Interface * const spi = boardSetupSpi();
#endif
  res = ifSetParam(spi, IF_RATE, &spiTestRate);
  assert(res == E_OK);

  struct Timer * const timer = boardSetupTimer();
  timerSetOverflow(timer, timerGetFrequency(timer) / 4);

  unsigned int value = 0;
  bool event = false;

#ifdef TEST_ZEROCOPY
  res = ifSetParam(spi, IF_ZEROCOPY, 0);
  assert(res == E_OK);
  ifSetCallback(spi, onTransferCompleted, &value);
#endif

  /* Suppress warning */
  (void)res;

  timerSetCallback(timer, onTimerOverflow, &event);
  timerEnable(timer);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    const uint32_t buffer = toBigEndian32(value);

    pinSet(led);
    pinReset(cs);
    ifWrite(spi, &buffer, sizeof(buffer));
    pinSet(cs);
    pinReset(led);

#ifndef TEST_ZEROCOPY
    ++value;
#endif
  }

  return 0;
}
