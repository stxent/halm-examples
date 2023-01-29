/*
 * lpc17xx_default/i2s/main.c
 * Copyright (C) 2021 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <xcore/stream.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
struct EventTuple
{
  struct Stream *stream;
  struct Pin led;
};
/*----------------------------------------------------------------------------*/
static const int16_t pattern[] = {
         0,   4663,   9231,  13611,
     17715,  21457,  24763,  27565,
     29805,  31439,  32433,  32767,
     32433,  31439,  29805,  27565,
     24763,  21457,  17715,  13611,
      9231,   4663,
         0,  -4663,  -9231, -13611,
    -17715, -21457, -24763, -27565,
    -29805, -31439, -32433, -32767,
    -32433, -31439, -29805, -27565,
    -24763, -21457, -17715, -13611,
     -9231,  -4663
};

#define BUFFER_COUNT  2
#define CHANNEL_COUNT 2
#define BUFFER_LENGTH (ARRAY_SIZE(pattern) * 8)

static int16_t rxBuffers[BUFFER_COUNT][BUFFER_LENGTH * CHANNEL_COUNT];
static int16_t txBuffers[BUFFER_COUNT][BUFFER_LENGTH * CHANNEL_COUNT];
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
  for (size_t buffer = 0; buffer < BUFFER_COUNT; ++buffer)
  {
    for (size_t index = 0; index < BUFFER_LENGTH; ++index)
    {
      txBuffers[buffer][index * 2 + 0] = +pattern[index % ARRAY_SIZE(pattern)];
      txBuffers[buffer][index * 2 + 1] = -pattern[index % ARRAY_SIZE(pattern)];
    }
  }

  memset(rxBuffers, 0, sizeof(rxBuffers));
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  boardSetupClockPll();
  fillBuffers();

  struct Pin rxLed = pinInit(BOARD_LED_0);
  pinOutput(rxLed, false);
  struct Pin txLed = pinInit(BOARD_LED_1);
  pinOutput(txLed, false);

  struct StreamPackage audio = boardSetupI2S();

  struct EventTuple rxContext = {
      .stream = audio.rx,
      .led = rxLed
  };
  struct EventTuple txContext = {
      .stream = audio.tx,
      .led = txLed
  };

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
  streamEnqueue(audio.rx, &rxRequests[0]);
  txRequests[0].length = txRequests[0].capacity;
  streamEnqueue(audio.tx, &txRequests[0]);

  rxRequests[1].length = 0;
  streamEnqueue(audio.rx, &rxRequests[1]);
  txRequests[1].length = txRequests[1].capacity;
  streamEnqueue(audio.tx, &txRequests[1]);

  while (1);
  return 0;
}
