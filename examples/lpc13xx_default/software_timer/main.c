/*
 * lpc13xx_default/software_timer/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/generic/software_timer.h>
#include <halm/pin.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/lpc13xx/clocking.h>
/*----------------------------------------------------------------------------*/
#define OUTPUT_PIN_0 PIN(1, 0)
#define OUTPUT_PIN_1 PIN(1, 1)
#define OUTPUT_PIN_2 PIN(1, 2)
/*----------------------------------------------------------------------------*/
static struct GpTimerConfig tickTimerConfig = {
    .frequency = 1000000,
    .channel = GPTIMER_CT32B0
};
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct CommonClockConfig mainClkConfig = {
    .source = CLOCK_EXTERNAL
};
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainClock, &mainClkConfig);
}
/*----------------------------------------------------------------------------*/
static void onTimer0Overflow(void *argument)
{
  static bool state = false;

  pinWrite(*(const struct Pin *)argument, state);
  state = !state;
}
/*----------------------------------------------------------------------------*/
static void onTimer1Overflow(void *argument)
{
  static bool state = false;

  pinWrite(*(const struct Pin *)argument, state);
  state = !state;
}
/*----------------------------------------------------------------------------*/
static void onTimer2Overflow(void *argument)
{
  static bool state = false;

  pinWrite(*(const struct Pin *)argument, state);
  state = !state;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

  struct Pin output0 = pinInit(OUTPUT_PIN_0);
  pinOutput(output0, false);
  struct Pin output1 = pinInit(OUTPUT_PIN_1);
  pinOutput(output1, false);
  struct Pin output2 = pinInit(OUTPUT_PIN_2);
  pinOutput(output2, false);

  struct Timer * const tickTimer = init(GpTimer, &tickTimerConfig);
  assert(tickTimer);
  timerSetOverflow(tickTimer, 1000);

  const struct SoftwareTimerFactoryConfig factoryConfig = {
      .timer = tickTimer
  };

  struct SoftwareTimerFactory * const timerFactory =
      init(SoftwareTimerFactory, &factoryConfig);
  assert(timerFactory);

  struct Timer * const timer0 = softwareTimerCreate(timerFactory);
  assert(timer0);
  timerSetCallback(timer0, onTimer0Overflow, &output0);
  timerSetOverflow(timer0, 199);
  timerEnable(timer0);

  struct Timer * const timer1 = softwareTimerCreate(timerFactory);
  assert(timer1);
  timerSetCallback(timer1, onTimer1Overflow, &output1);
  timerSetOverflow(timer1, 37);
  timerEnable(timer1);

  struct Timer * const timer2 = softwareTimerCreate(timerFactory);
  assert(timer2);
  timerSetCallback(timer2, onTimer2Overflow, &output2);
  timerSetOverflow(timer2, 97);
  timerEnable(timer2);

  timerEnable(tickTimer);

  while (1);

  return 0;
}
/*----------------------------------------------------------------------------*/
void __assert_func(const char *file __attribute__((unused)),
    int line __attribute__((unused)),
    const char *func __attribute__((unused)),
    const char *expr __attribute__((unused)))
{
  while (1);
}
