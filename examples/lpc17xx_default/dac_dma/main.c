/*
 * lpc17xx_default/dac_dma/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <stdlib.h>
#include <halm/pin.h>
#include <halm/platform/nxp/dac_dma.h>
#include <halm/platform/nxp/lpc17xx/clocking.h>
#include <xcore/bits.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_COUNT  2
#define BUFFER_SIZE   512

#define LED_PIN       PIN(1, 8)
/*----------------------------------------------------------------------------*/
static const struct DacDmaConfig dacConfig = {
    .rate = 96000,
    .value = 32768,
    .pin = PIN(0, 26),
    .dma = 0
};
/*----------------------------------------------------------------------------*/
static const uint16_t table[] = {
    0,     157,   629,   1410,
    2494,  3869,  5522,  7437,
    9597,  11980, 14562, 17321,
    20227, 23255, 26374, 29555,
    32767, 35979, 39160, 42279,
    45307, 48213, 50972, 53554,
    55937, 58097, 60012, 61665,
    63040, 64124, 64905, 65377,
    65535
};

static uint16_t buffers[BUFFER_COUNT * BUFFER_SIZE];
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct CommonClockConfig mainClkConfig = {
    .source = CLOCK_EXTERNAL
};
/*----------------------------------------------------------------------------*/
static unsigned int enqueue(struct Interface *dac, uint16_t *buffer,
    unsigned int iteration)
{
  size_t number;

  while (ifGetParam(dac, IF_PENDING, &number) == E_OK && number < BUFFER_COUNT)
  {
    const unsigned int index = iteration++ % BUFFER_COUNT;
    ifWrite(dac, buffer + index * BUFFER_SIZE, BUFFER_SIZE * sizeof(uint16_t));
  }

  return iteration;
}
/*----------------------------------------------------------------------------*/
static void fill(uint16_t *buffer)
{
  const size_t width = (ARRAY_SIZE(table) - 1) * 2;

  for (size_t index = 0; index < BUFFER_SIZE * 2; ++index)
    buffer[index] = table[abs(index % width - width / 2)];
}
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainClock, &mainClkConfig);
}
/*----------------------------------------------------------------------------*/
static void onConversionCompleted(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();
  fill(buffers);

  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  struct Interface * const dac = init(DacDma, &dacConfig);
  assert(dac);

  bool event = false;
  ifSetCallback(dac, onConversionCompleted, &event);

  unsigned int iteration = 0;

  /* Initial buffer filling */
  iteration = enqueue(dac, buffers, iteration);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    pinSet(led);
    iteration = enqueue(dac, buffers, iteration);
    pinReset(led);
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
