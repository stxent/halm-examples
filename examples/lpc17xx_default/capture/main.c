/*
 * lpc17xx_default/capture/main.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/lpc/gppwm.h>
#include <halm/platform/lpc/gptimer_capture.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define INPUT_PIN  PIN(1, 18)
#define OUTPUT_PIN PIN(1, 23)
/*----------------------------------------------------------------------------*/
static const struct GpTimerCaptureUnitConfig captureUnitConfig = {
    .frequency = 1000000,
    .channel = 1
};

static const struct GpPwmUnitConfig pwmUnitConfig = {
    .frequency = 1000,
    .resolution = 10,
    .channel = 0
};
/*----------------------------------------------------------------------------*/
static void onCaptureEvent(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
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

  boardSetupClockPll();

  const struct Pin ledCapture = pinInit(BOARD_LED_0);
  pinOutput(ledCapture, false);
  const struct Pin ledOverflow = pinInit(BOARD_LED_1);
  pinOutput(ledOverflow, false);

  struct GpPwmUnit * const pwmUnit = init(GpPwmUnit, &pwmUnitConfig);
  assert(pwmUnit);

  struct Pwm * const pwm = gpPwmCreate(pwmUnit, OUTPUT_PIN);
  assert(pwm);
  pwmSetEdges(pwm, 0, pwmGetResolution(pwm) / 2);
  pwmEnable(pwm);

  struct GpTimerCaptureUnit * const captureUnit = init(GpTimerCaptureUnit,
      &captureUnitConfig);
  assert(pwmUnit);

  struct Capture * const capture = gpTimerCaptureCreate(captureUnit, INPUT_PIN,
      PIN_RISING, PIN_PULLDOWN);
  assert(capture);

  uint32_t previousTime = 0;
  bool captureEvent = false;
  bool overflowEvent = false;

  timerSetCallback(captureUnit, onTimerOverflow, &overflowEvent);
  timerEnable(captureUnit);
  captureSetCallback(capture, onCaptureEvent, &captureEvent);
  captureEnable(capture);
  pwmEnable(pwm);

  while (1)
  {
    while (!captureEvent && !overflowEvent)
      barrier();

    if (captureEvent)
    {
      captureEvent = false;

      const uint32_t currentTime = captureGetValue(capture);
      const uint32_t period = currentTime - previousTime;

      if (period >= minPeriod && period <= maxPeriod)
        pinToggle(ledCapture);
      previousTime = currentTime;
    }

    if (overflowEvent)
    {
      overflowEvent = false;
      pinToggle(ledOverflow);
    }
  }

  return 0;
}
