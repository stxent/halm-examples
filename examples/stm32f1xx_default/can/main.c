/*
 * stm32f1xx_default/can/main.c
 * Copyright (C) 2019 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/generic/can.h>
#include <halm/pin.h>
#include <halm/platform/stm32/can.h>
#include <halm/platform/stm32/clocking.h>
#include <halm/platform/stm32/gptimer.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN PIN(PORT_C, 13)

/* Period between message groups in milliseconds */
#define GROUP_PERIOD 10000
/* Number of messages in each group */
#define GROUP_SIZE   1000
/*----------------------------------------------------------------------------*/
static const struct GpTimerConfig blinkTimerConfig = {
    .frequency = 1000,
    .channel = TIM2
};

static const struct GpTimerConfig eventTimerConfig = {
    .frequency = 1000,
    .channel = TIM3
};

static const struct GpTimerConfig chronoTimerConfig = {
    .frequency = 1000,
    .channel = TIM4
};

static struct CanConfig canConfig = {
    .timer = 0,
    .rate = 1000000,
    .rxBuffers = 4,
    .txBuffers = 4,
    .rx = PIN(PORT_B, 8),
    .tx = PIN(PORT_B, 9),
    .priority = 0,
    .channel = 0
};
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 8000000
};

static const struct MainPllConfig mainPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 1,
    .multiplier = 6
};

static const struct SystemClockConfig systemClockConfig = {
    .source = CLOCK_PLL
};

static const struct BusClockConfig ahbBusClockConfig = {
    .divisor = 1
};

static const struct BusClockConfig apbBusClockConfig = {
    .divisor = 2
};
/*----------------------------------------------------------------------------*/
static void onBlinkTimeout(void *argument)
{
  void * const * const array = argument;
  struct Timer * const timer = array[0];
  const struct Pin * const led = array[1];

  pinReset(*led);
  timerDisable(timer);
}
/*----------------------------------------------------------------------------*/
static void onEvent(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
static void sendMessageGroup(struct Interface *interface,
    struct Timer *timer, uint8_t flags, size_t length, size_t count)
{
  static const uint32_t INTERFACE_TIMEOUT = 1000;

  timerSetValue(timer, 0);

  for (size_t i = 0; i < count; ++i)
  {
    const struct CANStandardMessage message = {
        .timestamp = 0,
        .id = (uint32_t)i,
        .flags = flags,
        .length = length,
        .data = {1, 2, 3, 4, 5, 6, 7, 8}
    };

    while (ifWrite(interface, &message, sizeof(message)) != sizeof(message))
    {
      if (timerGetValue(timer) >= INTERFACE_TIMEOUT)
        return;
    }
  }
}
/*----------------------------------------------------------------------------*/
static void runTransmissionTest(struct Interface *interface,
    struct Timer *timer)
{
  sendMessageGroup(interface, timer, 0, 0, GROUP_SIZE);
  sendMessageGroup(interface, timer, 0, 8, GROUP_SIZE);
  sendMessageGroup(interface, timer, CAN_EXT_ID, 0, GROUP_SIZE);
  sendMessageGroup(interface, timer, CAN_EXT_ID, 8, GROUP_SIZE);
  sendMessageGroup(interface, timer, CAN_RTR, 0, GROUP_SIZE);
  sendMessageGroup(interface, timer, CAN_EXT_ID | CAN_RTR, 0, GROUP_SIZE);
}
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainPll, &mainPllConfig);
  while (!clockReady(MainPll));

  clockEnable(Apb1Clock, &apbBusClockConfig);
  clockEnable(Apb2Clock, &apbBusClockConfig);
  clockEnable(SystemClock, &systemClockConfig);

  clockEnable(MainClock, &ahbBusClockConfig);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

  struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  struct Timer * const blinkTimer = init(GpTimer, &blinkTimerConfig);
  assert(blinkTimer);
  timerSetOverflow(blinkTimer, 50);

  void *onBlinkTimeoutArguments[] = {blinkTimer, &led};
  timerSetCallback(blinkTimer, onBlinkTimeout, &onBlinkTimeoutArguments);

  struct Timer * const eventTimer = init(GpTimer, &eventTimerConfig);
  assert(eventTimer);
  timerSetOverflow(eventTimer, GROUP_PERIOD);

  struct Timer * const chronoTimer = init(GpTimer, &chronoTimerConfig);
  assert(chronoTimer);
  timerEnable(chronoTimer);

  canConfig.timer = chronoTimer;
  struct Interface * const can = init(Can, &canConfig);
  assert(can);
  ifSetParam(can, IF_CAN_ACTIVE, 0);

  bool canEvent = false;
  bool timerEvent = false;

  ifSetCallback(can, onEvent, &canEvent);
  timerSetCallback(eventTimer, onEvent, &timerEvent);
  timerEnable(eventTimer);

  while (1)
  {
    while (!timerEvent && !canEvent)
      barrier();

    if (timerEvent)
    {
      timerEvent = false;
      runTransmissionTest(can, chronoTimer);
    }

    if (canEvent)
    {
      canEvent = false;

      struct CANStandardMessage message;
      ifRead(can, &message, sizeof(message));

      pinSet(led);
      timerEnable(blinkTimer);
    }
  }

  return 0;
}
