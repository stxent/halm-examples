/*
 * m03x_default/adc/main.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/numicro/adc.h>
#include <halm/platform/numicro/clocking.h>
#include <halm/platform/numicro/gptimer.h>
#include <halm/platform/numicro/serial.h>
#include <assert.h>
#include <stdio.h>
/*----------------------------------------------------------------------------*/
#define ADC_RATE      10
#define BUFFER_LENGTH 128
#define LED_PIN       PIN(PORT_B, 14)
/*----------------------------------------------------------------------------*/
static const PinNumber adcPinArray[] = {
    PIN(PORT_B, 0), PIN(PORT_B, 3), PIN(PORT_B, 6), PIN(PORT_B, 9), 0
};

static const struct AdcConfig adcConfig = {
    .pins = adcPinArray,
    .event = ADC_TIMER,
    .channel = 0
};

static const struct SerialConfig serialConfig = {
    .rxLength = BUFFER_LENGTH,
    .txLength = BUFFER_LENGTH,
    .rate = 19200,
    .rx = PIN(PORT_A, 0),
    .tx = PIN(PORT_A, 1),
    .channel = 0
};

static const struct GpTimerConfig timerConfig = {
    .frequency = 1000000,
    .channel = 0,
    .trigger = {
        .adc = true
    }
};
/*----------------------------------------------------------------------------*/
static const struct ExtendedClockConfig adcClockConfig = {
    .source = CLOCK_PLL,
    .divisor = 4
};

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 32000000
};

static const struct ExtendedClockConfig mainClockConfig = {
    .source = CLOCK_PLL,
    .divisor = 2
};

static const struct PllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 4,
    .multiplier = 12
};
/*----------------------------------------------------------------------------*/
static void onConversionCompleted(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(AdcClock, &adcClockConfig);
  clockEnable(MainClock, &mainClockConfig);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);

  struct Interface * const serial = init(Serial, &serialConfig);
  assert(serial);

  struct Interface * const adc = init(Adc, &adcConfig);
  assert(adc);

  const size_t count = ARRAY_SIZE(adcPinArray) - 1;
  bool event = false;

  ifSetCallback(adc, onConversionCompleted, &event);
  timerSetOverflow(timer, timerGetFrequency(timer) / ADC_RATE);
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
