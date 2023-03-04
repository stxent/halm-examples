/*
 * lpc17xx_default/adc_dma_stream/main.c
 * Copyright (C) 2022 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/delay.h>
#include <halm/timer.h>
#include <xcore/interface.h>
#include <xcore/stream.h>
#include <stdio.h>
/*----------------------------------------------------------------------------*/
#define ADC_RATE 10

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
  uint16_t buffers[count][2];

  boardSetupClockPll();

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, BOARD_LED_INV);

  struct Timer * const adcTimer = boardSetupAdcTimer();
  struct Timer * const memTimer = boardSetupTimer();
  struct Interface * const serial = boardSetupSerial();

  struct StreamPackage adc = boardSetupAdcStream();
  struct EventTuple context = {
      .serial = serial,
      .stream = adc.rx,
      .led = led
  };
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

  streamEnqueue(adc.rx, &requests[0]);
  streamEnqueue(adc.rx, &requests[1]);

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
