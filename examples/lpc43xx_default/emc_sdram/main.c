/*
 * lpc43xx_default/emc_sdram/main.c
 * Copyright (C) 2025 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/lpc/emc_sdram.h>
#include <assert.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
#define MEMORY_SIZE (1024 * 1024)

typedef uint32_t EmcWord;
/*----------------------------------------------------------------------------*/
#if 1
/* MT48LC8M32 */
static const struct EmcSdramConfig emcSdramConfig = {
    .timings = {
        .refresh = 15625,
        .apr = 34,
        .mrd = 20, /* For 100 MHz clock */
        .ras = 42,
        .rc = 70,
        .rp = 20,
        .rrd = 14,
        .wr = 14,
        .xsr = 70
    },

    .width = {
        .bus = 32,
        .device = 32
    },

    .clocks = {true, false, true, false},
    .channel = 0,
    .latency = 2,
    .banks = 4,
    .columns = 9,
    .rows = 12
};
#else
/* AS4C16M16 */
static const struct EmcSdramConfig emcSdramConfig = {
    .timings = {
        .refresh = 7812,
        .apr = 35,
        .mrd = 14,
        .ras = 42,
        .rc = 63,
        .rp = 21,
        .rrd = 14,
        .wr = 14,
        .xsr = 65
    },

    .width = {
        .bus = 16,
        .device = 16
    },

    .clocks = {true, false, true, false},
    .channel = 0,
    .latency = 3,
    .banks = 4,
    .columns = 9,
    .rows = 13
};
#endif
/*----------------------------------------------------------------------------*/
static EmcWord patternOnes(size_t)
{
  return (EmcWord)0xFFFFFFFFUL;
}
/*----------------------------------------------------------------------------*/
static EmcWord patternOnesEven(size_t)
{
  return (EmcWord)0x55555555UL;
}
/*----------------------------------------------------------------------------*/
static EmcWord patternOnesOdd(size_t)
{
  return (EmcWord)0xAAAAAAAAUL;
}
/*----------------------------------------------------------------------------*/
static EmcWord patternSequential(size_t position)
{
  return (EmcWord)position;
}
/*----------------------------------------------------------------------------*/
static EmcWord patternZeros(size_t)
{
  return (EmcWord)0x00000000UL;
}
/*----------------------------------------------------------------------------*/
static bool test(void *base, size_t size, EmcWord (*pattern)(size_t))
{
  const size_t elements = size / sizeof(EmcWord);
  EmcWord * const address = base;
  bool passed = true;

  for (size_t position = 0; position < elements; ++position)
    address[position] = pattern(position);

  for (size_t position = 0; position < elements; ++position)
  {
    const EmcWord value = address[position];

    if (value != pattern(position))
      passed = false;
  }

  return passed;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  boardSetupClockPllCustom(96000000);

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, BOARD_LED_INV);

  struct EmcSdram * const memory = init(EmcSdram, &emcSdramConfig);
  assert(memory != NULL);

  void * const address = emcSdramAddress(memory);

  while (1)
  {
    bool passed;

    /* Fill with all ones */
    passed = test(address, MEMORY_SIZE, patternOnes);
    pinWrite(led, passed ? BOARD_LED_INV : !BOARD_LED_INV);

    /* Fill with all zeros */
    passed = test(address, MEMORY_SIZE, patternZeros);
    pinWrite(led, passed ? BOARD_LED_INV : !BOARD_LED_INV);

    /* Fill with 0xAA pattern */
    passed = test(address, MEMORY_SIZE, patternOnesOdd);
    pinWrite(led, passed ? BOARD_LED_INV : !BOARD_LED_INV);

    /* Fill with 0x55 pattern */
    passed = test(address, MEMORY_SIZE, patternOnesEven);
    pinWrite(led, passed ? BOARD_LED_INV : !BOARD_LED_INV);

    /* Fill with pseudorandom pattern */
    passed = test(address, MEMORY_SIZE, patternSequential);
    pinWrite(led, passed ? BOARD_LED_INV : !BOARD_LED_INV);
  }

  return 0;
}
