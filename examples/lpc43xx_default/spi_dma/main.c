/*
 * lpc43xx_default/spi_dma/main.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/generic/spi.h>
#include <halm/timer.h>
#include <xcore/memory.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
static void onTransferCompleted(void *argument)
{
  ++(*(unsigned int *)argument);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  static const uint32_t SPI_TEST_RATE = 400000;
  static const uint8_t SPI_TEST_MODE = 3;
  static const bool USE_ZEROCOPY = true;

  unsigned int value = 0;
  bool event = false;
  enum Result res;

  boardSetupClockPll();

  const struct Pin cs = pinInit(BOARD_SPI_CS);
  pinOutput(cs, true);
  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, BOARD_LED_INV);

  struct Interface * const spi = boardSetupSpiDma();
  res = ifSetParam(spi, IF_RATE, &SPI_TEST_RATE);
  assert(res == E_OK);
  res = ifSetParam(spi, IF_SPI_MODE, &SPI_TEST_MODE);
  assert(res == E_OK);

  if (USE_ZEROCOPY)
  {
    ifSetParam(spi, IF_ZEROCOPY, 0);
    ifSetCallback(spi, onTransferCompleted, &value);
  }

  struct Timer * const timer = boardSetupTimer();
  timerSetOverflow(timer, timerGetFrequency(timer) / 4);
  timerSetCallback(timer, onTimerOverflow, &event);
  timerEnable(timer);

  /* Suppress warning */
  (void)res;

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    const uint32_t buffer = toBigEndian32(value);

    pinToggle(led);
    pinReset(cs);
    ifWrite(spi, &buffer, sizeof(buffer));
    pinSet(cs);
    pinToggle(led);

    if (!USE_ZEROCOPY)
      ++value;
  }

  return 0;
}
