/*
 * lpc43xx_default/emc_sram/main.c
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/lpc/emc_sram.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define MEMORY_SIZE 8192

typedef uint32_t EmcWord;
/*----------------------------------------------------------------------------*/
static const struct EmcSramConfig emcSramConfig = {
    .timings = {
        .oe = 0,
        .rd = 70,
        .we = 30,
        .wr = 70
    },

    .width = {
        .address = 23,
        .data = 16
    },

    .channel = 0,
    .buffering = true,
    .useWriteEnable = true
};
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

  struct EmcSram * const memory = init(EmcSram, &emcSramConfig);
  assert(memory != NULL);

  void * const address = emcSramAddress(memory);

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
