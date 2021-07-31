/*
 * lpc17xx_default/dac_dma/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/dac_dma.h>
#include <assert.h>
#include <stdlib.h>
/*----------------------------------------------------------------------------*/
struct EventTuple
{
  struct Stream *stream;
  struct Pin led;
};

#define BUFFER_COUNT  2
#define BUFFER_LENGTH 512
#define LED_PIN       PIN(1, 8)
/*----------------------------------------------------------------------------*/
static const struct DacDmaConfig dacConfig = {
    .size = 2,
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

static uint16_t buffers[BUFFER_COUNT * BUFFER_LENGTH];
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_EXTERNAL
};
/*----------------------------------------------------------------------------*/
static void fill(uint16_t *buffer)
{
  const size_t width = (ARRAY_SIZE(table) - 1) * 2;

  for (size_t index = 0; index < BUFFER_LENGTH * 2; ++index)
    buffer[index] = table[abs(index % width - width / 2)];
}
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainClock, &mainClockConfig);
}
/*----------------------------------------------------------------------------*/
static void onConversionCompleted(void *argument, struct StreamRequest *request,
    enum StreamRequestStatus status __attribute__((unused)))
{
  struct EventTuple * const context = argument;

  request->length = request->capacity;
  streamEnqueue(context->stream, request);
  pinToggle(context->led);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();
  fill(buffers);

  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);
  const struct Pin auxpwr = pinInit(PIN(2, 8));
  pinOutput(auxpwr, false);

  struct DacDma * const dac = init(DacDma, &dacConfig);
  assert(dac);
  struct Stream * const stream = dacDmaGetOutput(dac);
  assert(stream);

  struct EventTuple context = {
      .stream = stream,
      .led = led
  };

  struct StreamRequest requests[2] = {
      {
          BUFFER_LENGTH * sizeof(uint16_t),
          0,
          onConversionCompleted,
          &context,
          buffers
      }, {
          BUFFER_LENGTH * sizeof(uint16_t),
          0,
          onConversionCompleted,
          &context,
          buffers + BUFFER_LENGTH
      }
  };

  /* Enqueue buffers */
  requests[0].length = requests[0].capacity;
  streamEnqueue(stream, &requests[0]);
  requests[1].length = requests[1].capacity;
  streamEnqueue(stream, &requests[1]);

  while (1);
  return 0;
}
