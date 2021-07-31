/*
 * lpc43xx_default/adc_dma/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/lpc/adc_dma.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/gptimer.h>
#include <halm/platform/lpc/serial.h>
#include <assert.h>
#include <stdio.h>
/*----------------------------------------------------------------------------*/
#define ADC_RATE      1
#define BUFFER_LENGTH 128
#define LED_PIN       PIN(PORT_7, 7)
/*----------------------------------------------------------------------------*/
static const PinNumber adcPinArray[] = {
    PIN(PORT_ADC, 1), PIN(PORT_ADC, 2), PIN(PORT_ADC, 3), PIN(PORT_ADC, 5), 0
};

static const struct AdcDmaConfig adcConfig = {
    .pins = adcPinArray,
    .event = ADC_CTOUT_15,
    .channel = 0,
    .dma = 0
};

static const struct SerialConfig serialConfig = {
    .rate = 19200,
    .rxLength = BUFFER_LENGTH,
    .txLength = BUFFER_LENGTH,
    .rx = PIN(2, 4),
    .tx = PIN(2, 3),
    .channel = 3
};

static const struct GpTimerConfig timerConfig = {
    .frequency = 4000000,
    .event = GPTIMER_MATCH3,
    .channel = 3
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
static void onConversionCompleted(void *argument)
{
  *(bool *)argument = true;
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
  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);

  struct Interface * const serial = init(Serial, &serialConfig);
  assert(serial);

  struct Interface * const adc = init(AdcDma, &adcConfig);
  assert(adc);

  const size_t count = ARRAY_SIZE(adcPinArray) - 1;
  bool event = false;

  ifSetCallback(adc, onConversionCompleted, &event);
  timerSetOverflow(timer, timerGetFrequency(timer) / (count * ADC_RATE * 2));
  timerEnable(timer);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    uint16_t buffer[count];
    char text[count * 6 + 3];
    char *iter = text;

    ifRead(adc, buffer, sizeof(buffer));

    for (size_t index = 0; index < count; ++index)
      iter += sprintf(iter, "%5u ", (unsigned int)buffer[index]);
    iter += sprintf(iter, "\r\n");
    ifWrite(serial, text, iter - text);

    pinToggle(led);
  }

  return 0;
}
