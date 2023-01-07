/*
 * lpc17xx_default/adc_dma_stream/main.c
 * Copyright (C) 2022 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/delay.h>
#include <halm/pin.h>
#include <halm/platform/lpc/adc_dma_stream.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/gptimer.h>
#include <halm/platform/lpc/serial.h>
#include <assert.h>
#include <stdio.h>
/*----------------------------------------------------------------------------*/
struct EventTuple
{
  struct Interface *serial;
  struct Stream *stream;
  struct Pin led;
};

#define ADC_RATE  8
#define LED_PIN   PIN(1, 8)
/*----------------------------------------------------------------------------*/
static const PinNumber adcPinArray[] = {
    PIN(0, 25), PIN(1, 31), PIN(0, 3), PIN(0, 2), 0
};

static const struct AdcDmaStreamConfig adcConfig = {
    .pins = adcPinArray,
    .size = 2,
    .converter = {ADC_TIMER1_MAT1, 0},
    .memory = {GPDMA_MAT0_0, 1},
    .channel = 0
};

static const struct SerialConfig serialConfig = {
    .rxLength = 16,
    .txLength = ((ARRAY_SIZE(adcPinArray) - 1) * 6 + 4) * ADC_RATE,
    .rate = 19200,
    .rx = PIN(0, 16),
    .tx = PIN(0, 15),
    .channel = 1
};

static const struct GpTimerConfig converterTimerConfig = {
    .frequency = 4000000,
    .event = GPTIMER_MATCH1,
    .channel = 1
};

static const struct GpTimerConfig memoryTimerConfig = {
    .frequency = 4000000,
    .event = GPTIMER_MATCH0,
    .channel = 0
};
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_EXTERNAL
};
/*----------------------------------------------------------------------------*/
static void onConversionCompleted(void *argument, struct StreamRequest *request,
    enum StreamRequestStatus status __attribute__((unused)))
{
  const size_t count = ARRAY_SIZE(adcPinArray) - 1;
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
static void setupClock(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainClock, &mainClockConfig);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  /*
  * The overflow frequency of the timer should be two times higher
  * than that of the hardware events for ADC.
  */
  struct Timer * const converterTimer = init(GpTimer, &converterTimerConfig);
  assert(converterTimer);
  struct Timer * const memoryTimer = init(GpTimer, &memoryTimerConfig);
  assert(memoryTimer);

  struct Interface * const serial = init(Serial, &serialConfig);
  assert(serial);

  struct AdcDmaStream * const adc = init(AdcDmaStream, &adcConfig);
  assert(adc);
  struct Stream * const stream = adcDmaStreamGetInput(adc);
  assert(stream);

  struct EventTuple context = {
      .serial = serial,
      .stream = stream,
      .led = led
  };

  const size_t count = ARRAY_SIZE(adcPinArray) - 1;
  uint16_t buffers[count * ADC_RATE][2];

  struct StreamRequest requests[2] = {
      {
          count * ADC_RATE * sizeof(uint16_t),
          0,
          onConversionCompleted,
          &context,
          &buffers[0]
      }, {
          count * ADC_RATE * sizeof(uint16_t),
          0,
          onConversionCompleted,
          &context,
          &buffers[1]
      }
  };

  streamEnqueue(stream, &requests[0]);
  streamEnqueue(stream, &requests[1]);

  /* Start conversion */
  timerSetOverflow(converterTimer,
      timerGetFrequency(converterTimer) / (count * ADC_RATE * 2));
  timerSetOverflow(memoryTimer,
      timerGetFrequency(memoryTimer) / (count * ADC_RATE));

  /* ADC conversion takes 5 us at 13 MHz */
  timerEnable(memoryTimer);
  udelay(5);
  timerEnable(converterTimer);

  while (1);
  return 0;
}
