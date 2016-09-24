/*
 * can/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>

#include <halm/generic/can.h>
#include <halm/pin.h>
#include <halm/platform/nxp/can.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/lpc17xx/clocking.h>
#include <xcore/bits.h>
/*----------------------------------------------------------------------------*/
#define TEST_EXT_ID
/* #define TEST_RTR */
#define TEST_TIMESTAMPS

/* Period between messages in microseconds */
#define MESSAGE_PERIOD 100000
/*----------------------------------------------------------------------------*/
static const struct GpTimerConfig blinkerConfig = {
    .frequency = 1000,
    .channel = 0
};

static const struct GpTimerConfig eventTimerConfig = {
    .frequency = 1000000,
    .channel = 1
};

#ifdef TEST_TIMESTAMPS
static const struct GpTimerConfig chronoTimerConfig = {
    .frequency = 1000000,
    .channel = 2
};
#endif

static struct CanConfig canConfig = {
    .rate = 1000000,
    .rxBuffers = 4,
    .txBuffers = 4,
    .rx = PIN(0, 0),
    .tx = PIN(0, 1),
    .priority = 0,
    .channel = 0
};
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct PllConfig sysPllConfig = {
    .multiplier = 30,
    .divisor = 6,
    .source = CLOCK_EXTERNAL
};

static const struct CommonClockConfig mainClkConfig = {
    .source = CLOCK_PLL
};
/*----------------------------------------------------------------------------*/
#ifndef TEST_RTR
static char binToHex(uint8_t value)
{
  const uint8_t nibble = value & 0x0F;

  return nibble < 10 ? nibble + '0' : nibble + 'A' - 10;
}
#endif
/*----------------------------------------------------------------------------*/
#ifndef TEST_RTR
static void numberToHex(uint8_t *output, uint32_t value)
{
  for (unsigned int i = 0; i < sizeof(value) * 2; ++i)
    *output++ = binToHex((uint8_t)(value >> 4 * i));
}
#endif
/*----------------------------------------------------------------------------*/
static void setupClock()
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &mainClkConfig);
}
/*----------------------------------------------------------------------------*/
static void onBlinkTimeout(void *argument)
{
  void * const * const array = argument;

  struct Timer * const blinker = array[0];
  const struct Pin * const pin = array[1];

  pinReset(*pin);
  timerSetEnabled(blinker, false);
}
/*----------------------------------------------------------------------------*/
static void onEvent(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  struct Pin led = pinInit(PIN(0, 22));
  pinOutput(led, 0);

  setupClock();

  struct Timer * const blinker = init(GpTimer, &blinkerConfig);
  assert(blinker);
  timerSetOverflow(blinker, 50);

  void *blinkTimeoutHandlerArguments[] = {
      blinker,
      &led
  };
  timerCallback(blinker, onBlinkTimeout, &blinkTimeoutHandlerArguments);

  struct Timer * const eventTimer = init(GpTimer, &eventTimerConfig);
  assert(eventTimer);
  timerSetOverflow(eventTimer, MESSAGE_PERIOD);

#ifdef TEST_TIMESTAMPS
  struct Timer * const chronoTimer = init(GpTimer, &chronoTimerConfig);
  assert(chronoTimer);
  timerSetEnabled(chronoTimer, true);
#else
  struct Timer * const chronoTimer = 0;
#endif

  canConfig.timer = chronoTimer;

  struct Interface * const can = init(Can, &canConfig);
  assert(can);
  ifSet(can, IF_CAN_ACTIVE, 0);

  bool canEvent = false;
  bool timerEvent = false;

  ifCallback(can, onEvent, &canEvent);
  timerCallback(eventTimer, onEvent, &timerEvent);
  timerSetEnabled(eventTimer, true);

  unsigned int iteration = 0;

  while (1)
  {
    while (!timerEvent && !canEvent)
      barrier();

    if (timerEvent)
    {
      timerEvent = false;

      struct CanStandardMessage message = {
          .timestamp = 0,
          .flags = 0,
          .data = {0}
      };

#ifdef TEST_EXT_ID
      message.id = iteration & MASK(29);
      message.flags |= CAN_EXT_ID;
#else
      message.id = iteration & MASK(11);
#endif

#ifdef TEST_RTR
      message.length = 0;
      message.flags |= CAN_RTR;
#else
      message.length = iteration % 9;
      numberToHex(message.data, iteration);
#endif

      ++iteration;

      ifWrite(can, &message, sizeof(message));
    }

    if (canEvent)
    {
      canEvent = false;

      struct CanStandardMessage message;
      ifRead(can, &message, sizeof(message));

      pinSet(led);
      timerSetEnabled(blinker, true);
    }
  }

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
