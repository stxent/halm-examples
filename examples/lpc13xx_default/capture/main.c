/*
 * lpc13xx_default/capture/main.c
 * Copyright (C) 2018 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/nxp/gptimer_pwm.h>
#include <halm/platform/nxp/gptimer_capture.h>
#include <halm/platform/nxp/lpc13xx/clocking.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define INPUT_PIN  PIN(1, 0)
#define OUTPUT_PIN PIN(1, 10)
#define LED_PIN    PIN(3, 0)
/*----------------------------------------------------------------------------*/
static const struct GpTimerCaptureUnitConfig captureUnitConfig = {
    .frequency = 1000000,
    .channel = GPTIMER_CT32B1
};

static const struct GpTimerPwmUnitConfig pwmUnitConfig = {
    .frequency = 1000,
    .resolution = 10,
    .channel = GPTIMER_CT16B1
};
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_EXTERNAL
};
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainClock, &mainClockConfig);
}
/*----------------------------------------------------------------------------*/
static void onCaptureEvent(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  const uint32_t maxPeriod =
      captureUnitConfig.frequency / pwmUnitConfig.frequency + 1;
  const uint32_t minPeriod =
      captureUnitConfig.frequency / pwmUnitConfig.frequency - 1;

  setupClock();

  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  struct GpTimerPwmUnit * const pwmUnit = init(GpTimerPwmUnit,
      &pwmUnitConfig);
  assert(pwmUnit);

  struct Pwm * const pwm = gpTimerPwmCreate(pwmUnit, OUTPUT_PIN);
  assert(pwm);
  pwmSetEdges(pwm, 0, pwmGetResolution(pwm) / 2);

  struct GpTimerCaptureUnit * const captureUnit = init(GpTimerCaptureUnit,
      &captureUnitConfig);
  assert(pwmUnit);

  struct Capture * const capture = gpTimerCaptureCreate(captureUnit, INPUT_PIN,
      PIN_RISING, PIN_PULLDOWN);
  assert(capture);

  uint32_t previousTime = 0;
  bool event = false;

  captureSetCallback(capture, onCaptureEvent, &event);
  captureEnable(capture);
  pwmEnable(pwm);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    const uint32_t currentTime = captureGetValue(capture);
    const uint32_t period = currentTime - previousTime;

    if (period >= minPeriod && period <= maxPeriod)
      pinReset(led);
    else
      pinSet(led);

    previousTime = currentTime;
  }

  return 0;
}
