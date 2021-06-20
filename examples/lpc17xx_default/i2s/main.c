/*
 * lpc17xx_default/i2s/main.c
 * Copyright (C) 2021 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/i2s_dma.h>
#include <assert.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
struct EventTuple
{
  struct Stream *stream;
  struct Pin led;
};

#define CHANNEL_COUNT 2

#define BUFFER_COUNT  2
#define BUFFER_LENGTH 128

#define LED_RX_PIN    PIN(1, 9)
#define LED_TX_PIN    PIN(1, 10)
/*----------------------------------------------------------------------------*/
static int16_t rxBuffers[BUFFER_COUNT][BUFFER_LENGTH * CHANNEL_COUNT];
static int16_t txBuffers[BUFFER_COUNT][BUFFER_LENGTH * CHANNEL_COUNT];
/*----------------------------------------------------------------------------*/
static const struct I2SDmaConfig i2sConfig = {
    .rate = 8000,
    .size = 2,
    .width = I2S_WIDTH_16,
    .channel = 0,
    .mono = false,
    .slave = false,
    .tx = {
        .sda = PIN(0, 9),
        .sck = PIN(0, 7),
        .ws = PIN(0, 8),
        .mclk = PIN(4, 29),
        .dma = 0
    },
    .rx = {
        .sda = PIN(0, 6),
        .dma = 1
    }
};
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct PllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 3,
    .multiplier = 25
};

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_PLL
};
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &mainClockConfig);
}
/*----------------------------------------------------------------------------*/
static void onDataReceived(void *argument, struct StreamRequest *request,
    enum StreamRequestStatus status __attribute__((unused)))
{
  struct EventTuple * const context = argument;

  request->length = 0;
  streamEnqueue(context->stream, request);
  pinToggle(context->led);
}
/*----------------------------------------------------------------------------*/
static void onDataSent(void *argument, struct StreamRequest *request,
    enum StreamRequestStatus status __attribute__((unused)))
{
  struct EventTuple * const context = argument;

  request->length = request->capacity;
  streamEnqueue(context->stream, request);
  pinToggle(context->led);
}
/*----------------------------------------------------------------------------*/
static void fillBuffers(void)
{
  for (size_t index = 0; index < BUFFER_LENGTH; ++index)
  {
    txBuffers[0][index * 2 + 0] = -100 - index;
    txBuffers[0][index * 2 + 1] = +100 + index;
  }

  for (size_t index = 0; index < BUFFER_LENGTH; ++index)
  {
    txBuffers[1][index * 2 + 0] = -10000 - index;
    txBuffers[1][index * 2 + 1] = +10000 + index;
  }

  memset(rxBuffers, 0, sizeof(rxBuffers));
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

  struct Pin rxLed = pinInit(LED_RX_PIN);
  pinOutput(rxLed, false);
  struct Pin txLed = pinInit(LED_TX_PIN);
  pinOutput(txLed, false);

  struct I2SDma * const audio = init(I2SDma, &i2sConfig);
  assert(audio);
  struct Stream * const rxStream = i2sDmaGetInput(audio);
  assert(rxStream);
  struct Stream * const txStream = i2sDmaGetOutput(audio);
  assert(txStream);

  struct EventTuple rxContext = {
      .stream = rxStream,
      .led = rxLed
  };
  struct EventTuple txContext = {
      .stream = txStream,
      .led = txLed
  };

  fillBuffers();

  struct StreamRequest rxRequests[2] = {
      {
          BUFFER_LENGTH * CHANNEL_COUNT * sizeof(int16_t),
          0,
          onDataReceived,
          &rxContext,
          &rxBuffers[0]
      }, {
          BUFFER_LENGTH * CHANNEL_COUNT * sizeof(int16_t),
          0,
          onDataReceived,
          &rxContext,
          &rxBuffers[1]
      }
  };
  struct StreamRequest txRequests[2] = {
      {
          BUFFER_LENGTH * CHANNEL_COUNT * sizeof(int16_t),
          0,
          onDataSent,
          &txContext,
          &txBuffers[0]
      }, {
          BUFFER_LENGTH * CHANNEL_COUNT * sizeof(int16_t),
          0,
          onDataSent,
          &txContext,
          &txBuffers[1]
      }
  };

  /* Enqueue buffers */
  rxRequests[0].length = 0;
  streamEnqueue(rxStream, &rxRequests[0]);
  txRequests[0].length = txRequests[0].capacity;
  streamEnqueue(txStream, &txRequests[0]);

  rxRequests[1].length = 0;
  streamEnqueue(rxStream, &rxRequests[1]);
  txRequests[1].length = txRequests[1].capacity;
  streamEnqueue(txStream, &txRequests[1]);

  while (1);
  return 0;
}
