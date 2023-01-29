/*
 * lpc17xx_default/capture/main.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/capture.h>
#include <halm/pwm.h>
#include <halm/timer.h>
#include <xcore/memory.h>
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
  uint32_t previousTime = 0;
  bool captureEvent = false;
  bool overflowEvent = false;

  boardSetupClockExt();

  const struct Pin ledCapture = pinInit(BOARD_LED_0);
  pinOutput(ledCapture, false);
  const struct Pin ledOverflow = pinInit(BOARD_LED_1);
  pinOutput(ledOverflow, false);

  struct CapturePackage capture = boardSetupCapture();
  captureSetCallback(capture.input, onCaptureEvent, &captureEvent);
  captureEnable(capture.input);
  timerSetOverflow(capture.timer, timerGetFrequency(capture.timer) * 5);
  timerSetCallback(capture.timer, onTimerOverflow, &overflowEvent);
  timerEnable(capture.timer);

  struct PwmPackage pwm = boardSetupPwm();
  pwmSetEdges(pwm.output, 0, timerGetOverflow(pwm.timer) / 2);
  pwmEnable(pwm.output);
  timerEnable(pwm.timer);

  const uint32_t CAPTURE_FREQUENCY =
      timerGetFrequency(capture.timer);
  const uint32_t PWM_FREQUENCY =
      timerGetFrequency(pwm.timer) / timerGetOverflow(pwm.timer);
  const uint32_t MAX_PERIOD = CAPTURE_FREQUENCY / PWM_FREQUENCY + 1;
  const uint32_t MIN_PERIOD = CAPTURE_FREQUENCY / PWM_FREQUENCY - 1;

  while (1)
  {
    while (!captureEvent && !overflowEvent)
      barrier();

    if (captureEvent)
    {
      captureEvent = false;

      const uint32_t currentTime = captureGetValue(capture.input);
      const uint32_t period = currentTime - previousTime;

      if (period >= MIN_PERIOD && period <= MAX_PERIOD)
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
