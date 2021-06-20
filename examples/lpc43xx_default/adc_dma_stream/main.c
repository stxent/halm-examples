/*
 * lpc43xx_default/adc_dma_stream/main.c
 * Copyright (C) 2021 xent
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
  struct Stream *adc;
  struct Pin led;
};

#define ADC_RATE  8
#define LED_PIN   PIN(PORT_7, 7)
/*----------------------------------------------------------------------------*/
static const PinNumber adcPinArray[] = {
    PIN(PORT_ADC, 1), PIN(PORT_ADC, 2), PIN(PORT_ADC, 3), PIN(PORT_ADC, 5), 0
};

static const struct AdcDmaStreamConfig adcConfig = {
    .pins = adcPinArray,
    .size = 2,
    .converter = {ADC_CTOUT_15, 0},
    .memory = {GPDMA_MAT0_0, 1},
    .channel = 0
};

static const struct SerialConfig serialConfig = {
    .rate = 19200,
    .rxLength = 16,
    .txLength = ((ARRAY_SIZE(adcPinArray) - 1) * 6 + 4) * ADC_RATE,
    .rx = PIN(2, 4),
    .tx = PIN(2, 3),
    .channel = 3
};

static const struct GpTimerConfig converterTimerConfig = {
    .frequency = 4000000,
    .event = GPTIMER_MATCH3,
    .channel = 3
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

static const struct GenericClockConfig initialClockConfig = {
    .source = CLOCK_INTERNAL
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
  streamEnqueue(context->adc, request);
}
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(MainClock, &initialClockConfig);

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainClock, &mainClockConfig);

  clockEnable(Apb3Clock, &mainClockConfig);
  while (!clockReady(Apb3Clock));

  clockEnable(Usart3Clock, &mainClockConfig);
  while (!clockReady(Usart3Clock));
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
      .adc = stream,
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

  /* ADC conversion takes 2.75 us at 4 MHz */
  timerEnable(memoryTimer);
  udelay(3);
  timerEnable(converterTimer);

  while (1);
  return 0;
}
