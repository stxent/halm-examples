/*
 * lpc43xx_default/sct/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/pin.h>
#include <halm/platform/nxp/sct_timer.h>
#include <halm/platform/nxp/lpc43xx/clocking.h>
/*----------------------------------------------------------------------------*/
struct Descriptor
{
  struct Pin led;
  bool state;
} descriptors[2];
/*----------------------------------------------------------------------------*/
#define LED_PIN_A PIN(PORT_6, 6)
#define LED_PIN_B PIN(PORT_6, 7)

#define TEST_UNIFIED
/*----------------------------------------------------------------------------*/
#ifdef TEST_UNIFIED
static const struct SctTimerConfig timerConfig = {
    .frequency = 1000000,
    .part = SCT_UNIFIED,
    .channel = 0
};
#else
static const struct SctTimerConfig timerConfigs[] = {
    {
        .frequency = 100000,
        .part = SCT_LOW,
        .channel = 0
    },
    {
        .frequency = 400000,
        .part = SCT_HIGH,
        .channel = 0
    }
};
#endif

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_INTERNAL
};
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(MainClock, &mainClockConfig);
}
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  struct Descriptor * const descriptor = argument;

  pinWrite(descriptor->led, descriptor->state);
  descriptor->state = !descriptor->state;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

  descriptors[0] = (struct Descriptor){
    pinInit(LED_PIN_A),
    false
  };
  descriptors[1] = (struct Descriptor){
    pinInit(LED_PIN_B),
    false
  };

  pinOutput(descriptors[0].led, false);
  pinOutput(descriptors[1].led, false);

#ifdef TEST_UNIFIED
  struct Timer * const timerA = init(SctUnifiedTimer, &timerConfig);
  assert(timerA);
  timerSetOverflow(timerA, 500000);
  timerSetCallback(timerA, onTimerOverflow, &descriptors[0]);
  timerEnable(timerA);
#else
  struct Timer * const timerA = init(SctTimer, &timerConfigs[0]);
  assert(timerA);
  timerSetOverflow(timerA, 50000);
  timerSetCallback(timerA, onTimerOverflow, &descriptors[0]);
  timerEnable(timerA);

  struct Timer * const timerB = init(SctTimer, &timerConfigs[1]);
  assert(timerB);
  timerSetOverflow(timerB, 50000);
  timerSetCallback(timerB, onTimerOverflow, &descriptors[1]);
  timerEnable(timerB);
#endif

  while (1);
  return 0;
}
