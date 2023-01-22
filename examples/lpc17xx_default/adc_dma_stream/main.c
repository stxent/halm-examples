/*
 * lpc17xx_default/adc_dma_stream/main.c
 * Copyright (C) 2022 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/delay.h>
#include <halm/platform/lpc/adc_dma_stream.h>
#include <halm/platform/lpc/gptimer.h>
#include <assert.h>
#include <stdio.h>
/*----------------------------------------------------------------------------*/
#define ADC_RATE 8

struct EventTuple
{
  struct Interface *serial;
  struct Stream *stream;
  struct Pin led;
};
/*----------------------------------------------------------------------------*/
static void onConversionCompleted(void *argument, struct StreamRequest *request,
    enum StreamRequestStatus status __attribute__((unused)))
{
  const size_t count = boardGetAdcPinCount();
  const size_t chunks = (request->length >> 1) / count;

  struct EventTuple * const context = argument;
  const uint16_t *buffer = request->buffer;

  pinToggle(context->led);
  ifWrite(context->serial, "\r\n", 2);

  for (size_t chunk = 0; chunk < chunks; ++chunk)
  {
    char text[count * 6 + 3];
    char *iter = text;

    for (size_t index = 0; index < count; ++index)
    {
      iter += sprintf(iter, "%5u ", (unsigned int)(*buffer));
      ++buffer;
    }

    iter += sprintf(iter, "\r\n");
    ifWrite(context->serial, text, iter - text);
  }

  request->length = 0;
  streamEnqueue(context->stream, request);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  const size_t count = ADC_RATE * boardGetAdcPinCount();

  boardSetupClockPll();

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, false);

  struct Timer * const adcTimer = boardSetupAdcTimer();
  struct Timer * const memTimer = boardSetupTimer();
  struct Interface * const serial = boardSetupSerial();

  struct AdcDmaStream * const adc =
      (struct AdcDmaStream *)boardSetupAdcStream();
  struct Stream * const stream = adcDmaStreamGetInput(adc);
  assert(stream);

  struct EventTuple context = {
      .serial = serial,
      .stream = stream,
      .led = led
  };

  uint16_t buffers[count][2];

  struct StreamRequest requests[2] = {
      {
          count * sizeof(uint16_t),
          0,
          onConversionCompleted,
          &context,
          &buffers[0]
      }, {
          count * sizeof(uint16_t),
          0,
          onConversionCompleted,
          &context,
          &buffers[1]
      }
  };

  streamEnqueue(stream, &requests[0]);
  streamEnqueue(stream, &requests[1]);

  /*
   * The overflow frequency of the timer should be two times higher
   * than that of the hardware events for ADC.
   */
  timerSetOverflow(adcTimer, timerGetFrequency(adcTimer) / count / 2);
  timerSetOverflow(memTimer, timerGetFrequency(memTimer) / count);

  /* ADC conversion takes 5 us at 13 MHz */
  timerEnable(memTimer);
  udelay(5);
  timerEnable(adcTimer);

  while (1);
  return 0;
}
