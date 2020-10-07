/*
 * lpc43xx_default/emc_sram/main.c
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/nxp/emc_sram.h>
#include <halm/platform/nxp/lpc43xx/clocking.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define MEMORY_SIZE 8192
#define LED_PIN     PIN(PORT_6, 6)

typedef uint32_t EmcWord;
/*----------------------------------------------------------------------------*/
static const struct GenericClockConfig initialClockConfig = {
    .source = CLOCK_INTERNAL
};

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_IDIVE
};

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000,
    .bypass = false
};

static const struct GenericDividerConfig dividerConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 1
};

static const struct EmcSramConfig emcSramConfig = {
    .addressWidth = 13,
    .dataWidth = 8,

    .timings = {
        .oe = 0,
        .rc = 30,
        .wc = 30,
        .we = 20
    },

    .channel = 0,
    .partitioned = false
};
/*----------------------------------------------------------------------------*/
static EmcWord patternOnes(size_t position __attribute__((unused)))
{
  return (EmcWord)0xFFFFFFFFUL;
}
/*----------------------------------------------------------------------------*/
static EmcWord patternOnesEven(size_t position __attribute__((unused)))
{
  return (EmcWord)0x55555555UL;
}
/*----------------------------------------------------------------------------*/
static EmcWord patternOnesOdd(size_t position __attribute__((unused)))
{
  return (EmcWord)0xAAAAAAAAUL;
}
/*----------------------------------------------------------------------------*/
static EmcWord patternSequential(size_t position)
{
  return (EmcWord)position;
}
/*----------------------------------------------------------------------------*/
static EmcWord patternZeros(size_t position __attribute__((unused)))
{
  return (EmcWord)0x00000000UL;
}
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(MainClock, &initialClockConfig);

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(DividerE, &dividerConfig);
  while (!clockReady(DividerE));

  clockEnable(MainClock, &mainClockConfig);
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
  setupClock();

  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  struct EmcSram * const memory = init(EmcSram, &emcSramConfig);
  assert(memory);
  (void)memory;

  void * const address = emcSramAddress(memory);

  while (1)
  {
    bool passed;

    /* Fill with all ones */
    passed = test(address, MEMORY_SIZE, patternOnes);
    pinWrite(led, !passed);

    /* Fill with all zeros */
    passed = test(address, MEMORY_SIZE, patternZeros);
    pinWrite(led, !passed);

    /* Fill with 0xAA pattern */
    passed = test(address, MEMORY_SIZE, patternOnesOdd);
    pinWrite(led, !passed);

    /* Fill with 0x55 pattern */
    passed = test(address, MEMORY_SIZE, patternOnesEven);
    pinWrite(led, !passed);

    /* Fill with pseudorandom pattern */
    passed = test(address, MEMORY_SIZE, patternSequential);
    pinWrite(led, !passed);
  }

  return 0;
}
