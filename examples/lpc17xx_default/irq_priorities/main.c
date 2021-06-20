/*
 * irq_priorities/main.c
 * Copyright (C) 2015 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/core/cortex/nvic.h>
#include <halm/delay.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/gptimer.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN_A PIN(1, 8)
#define LED_PIN_B PIN(1, 9)

/* #define ENABLE_GROUPING */
/*----------------------------------------------------------------------------*/
static const struct GpTimerConfig lowPriTimerConfig = {
    .frequency = 1000,
    .channel = 0,
    .priority = 1
};

static const struct GpTimerConfig highPriTimerConfig = {
    .frequency = 1000,
    .channel = 1,
    .priority = 3
};

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_EXTERNAL
};
/*----------------------------------------------------------------------------*/
static void lowPriCallback(void *argument)
{
  struct Pin * const pin = argument;

  pinSet(*pin);
  mdelay(1000);
  pinReset(*pin);
}
/*----------------------------------------------------------------------------*/
static void highPriCallback(void *argument)
{
  struct Pin * const pin = argument;

  pinSet(*pin);
  mdelay(100);
  pinReset(*pin);
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

  struct Pin led[] = {
      pinInit(LED_PIN_A),
      pinInit(LED_PIN_B)
  };

  pinOutput(led[0], false);
  pinOutput(led[1], false);

  const uint8_t groups = nvicGetPriorityGrouping();

#ifndef ENABLE_GROUPING
  /* LED's will flash independently */
  nvicSetPriorityGrouping(groups + 1);
#else
  /* Enabling of the led[0] will stop flashing of the led[1] */
  nvicSetPriorityGrouping(groups + 2);
#endif

  struct Timer * const lowPriTimer = init(GpTimer, &lowPriTimerConfig);
  assert(lowPriTimer);
  timerSetOverflow(lowPriTimer, 4000);
  timerSetCallback(lowPriTimer, lowPriCallback, &led[0]);

  struct Timer * const highPriTimer = init(GpTimer, &highPriTimerConfig);
  assert(highPriTimer);
  timerSetOverflow(highPriTimer, 200);
  timerSetCallback(highPriTimer, highPriCallback, &led[1]);

  timerEnable(lowPriTimer);
  timerEnable(highPriTimer);

  while (1);
  return 0;
}
