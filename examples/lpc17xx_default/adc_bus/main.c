/*
 * lpc17xx_default/adc_bus/main.c
 * Copyright (C) 2021 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/lpc/adc_bus.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/gptimer.h>
#include <halm/platform/lpc/serial.h>
#include <assert.h>
#include <stdio.h>
/*----------------------------------------------------------------------------*/
#define ADC_RATE      50
#define BUFFER_LENGTH 128
#define LED_PIN       PIN(1, 8)
/*----------------------------------------------------------------------------*/
static const PinNumber adcPinArray[] = {
    PIN(0, 25), PIN(1, 31), PIN(0, 3), PIN(0, 2), 0
};

static const struct AdcBusConfig adcConfig = {
    .pins = adcPinArray,
    .event = ADC_TIMER1_MAT1,
    .channel = 0
};

static const struct SerialConfig serialConfig = {
    .rate = 19200,
    .rxLength = BUFFER_LENGTH,
    .txLength = BUFFER_LENGTH,
    .rx = PIN(0, 16),
    .tx = PIN(0, 15),
    .channel = 1
};

static const struct GpTimerConfig timerConfig = {
    .frequency = 4000000,
    .event = GPTIMER_MATCH1,
    .channel = 1
};
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_EXTERNAL
};
/*----------------------------------------------------------------------------*/
static void onConversionCompleted(void *argument)
{
  *(bool *)argument = false;
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
  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);

  struct Interface * const serial = init(Serial, &serialConfig);
  assert(serial);

  struct Interface * const adc = init(AdcBus, &adcConfig);
  assert(adc);

  const enum Result res = ifSetParam(adc, IF_ZEROCOPY, 0);
  assert(res == E_OK);
  (void)res;

  const size_t count = ARRAY_SIZE(adcPinArray) - 1;
  uint16_t buffer[count];
  bool busy = true;

  ifSetCallback(adc, onConversionCompleted, &busy);
  timerSetOverflow(timer, timerGetFrequency(timer) / (count * ADC_RATE * 2));
  timerEnable(timer);

  /* Start conversion */
  ifRead(adc, buffer, sizeof(buffer));

  while (1)
  {
    while (busy)
      barrier();
    busy = true;

    char text[count * 6 + 3];
    char *iter = text;

    for (size_t index = 0; index < count; ++index)
      iter += sprintf(iter, "%5u ", (unsigned int)buffer[index]);
    iter += sprintf(iter, "\r\n");
    ifWrite(serial, text, iter - text);

    pinToggle(led);
    ifRead(adc, buffer, sizeof(buffer));
  }

  return 0;
}
