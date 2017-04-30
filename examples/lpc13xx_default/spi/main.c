/*
 * main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/pin.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/spi.h>
#include <xcore/memory.h>
/*----------------------------------------------------------------------------*/
#define CS_PIN  PIN(0, 2)
#define LED_PIN PIN(3, 0)

#define TEST_ZEROCOPY
/*----------------------------------------------------------------------------*/
static const struct GpTimerConfig timerConfig = {
    .frequency = 1000,
    .channel = GPTIMER_CT32B0
};
/*----------------------------------------------------------------------------*/
static const struct SpiConfig spiConfig = {
    .rate = 400000,
    .miso = PIN(0, 8),
    .mosi = PIN(0, 9),
    .sck = PIN(2, 11),
    .channel = 0,
    .mode = 0
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

  struct Interface * const spi = init(Spi, &spiConfig);
  assert(spi);

  static const uint32_t desiredRate = 200000;
  enum Result res;

  res = ifSetParam(spi, IF_RATE, &desiredRate);
  assert(res == E_OK);

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  timerSetOverflow(timer, 250);

  unsigned int value = 0;
  bool event = false;

#ifdef TEST_ZEROCOPY
  res = ifSetParam(spi, IF_ZEROCOPY, 0);
  assert(res == E_OK);
  res = ifSetCallback(spi, onTransferCompleted, &value);
  assert(res == E_OK);
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
