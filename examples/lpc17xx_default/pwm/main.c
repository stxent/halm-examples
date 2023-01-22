/*
 * lpc17xx_default/pwm/main.c
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/lpc/gppwm.h>
#include <halm/timer.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define DOUBLE_EDGE_PIN     PIN(1, 18)
#define SINGLE_EDGE_PIN     PIN(1, 26)
#define SINGLE_EDGE_REF_PIN PIN(1, 23)
/*----------------------------------------------------------------------------*/
static struct Pwm *doubleEdge;
static struct Pwm *singleEdge;
static struct Pwm *singleEdgeRef;
/*----------------------------------------------------------------------------*/
static const struct GpPwmUnitConfig pwmUnitConfig = {
    .frequency = 1000,
    .resolution = 10,
    .channel = 0
};
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *arg __attribute__((unused)))
{
  static uint32_t iteration = 0;

  const uint32_t resolution = pwmGetResolution(singleEdgeRef);
  const uint32_t duration = (iteration / 100) % (resolution + 1);
  const uint32_t trailing = duration < resolution ?
      ((iteration + duration) % resolution) : resolution;

  pwmSetEdges(singleEdge, 0, iteration % (resolution + 1));
  pwmSetEdges(doubleEdge, iteration % resolution, trailing);
  ++iteration;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  boardSetupClockPll();

  struct GpPwmUnit * const pwmUnit = init(GpPwmUnit, &pwmUnitConfig);
  assert(pwmUnit);

  singleEdgeRef = gpPwmCreate(pwmUnit, SINGLE_EDGE_REF_PIN);
  assert(singleEdgeRef);
  pwmSetEdges(singleEdgeRef, 0, pwmGetResolution(singleEdgeRef) / 2);
  pwmEnable(singleEdgeRef);

  singleEdge = gpPwmCreate(pwmUnit, SINGLE_EDGE_PIN);
  assert(singleEdge);
  pwmSetEdges(singleEdge, 0, 0);
  pwmEnable(singleEdge);

  doubleEdge = gpPwmCreateDoubleEdge(pwmUnit, DOUBLE_EDGE_PIN);
  assert(doubleEdge);
  pwmSetEdges(doubleEdge, 0, 0);
  pwmEnable(doubleEdge);

  struct Timer * const timer = boardSetupTimer();
  timerSetOverflow(timer, timerGetFrequency(timer) / 100);
  timerSetCallback(timer, onTimerOverflow, 0);
  timerEnable(timer);

  while (1);
  return 0;
}
