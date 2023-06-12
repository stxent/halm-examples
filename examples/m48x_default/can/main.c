/*
 * m48x_default/can/main.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/generic/can.h>
#include <halm/timer.h>
#include <xcore/memory.h>
/*----------------------------------------------------------------------------*/
/* Period between message groups in seconds */
#define GROUP_RATE  10
/* Number of messages in each group */
#define GROUP_SIZE  1000
/*----------------------------------------------------------------------------*/
static void onEvent(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
static void sendMessageGroup(struct Interface *interface,
    struct Timer *timer, uint8_t flags, size_t length, size_t count)
{
  /* Timeout is set to 1 second */
  const uint32_t timeout = timerGetFrequency(timer);

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
      if (timerGetValue(timer) >= timeout)
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
int main(void)
{
  bool canEvent = false;
  bool timerEvent = false;

  boardSetupClockPll();

  struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, BOARD_LED_INV);

  struct Timer * const chronoTimer = boardSetupTimer();
  timerEnable(chronoTimer);

  struct Timer * const eventTimer = boardSetupAdcTimer();
  timerSetOverflow(eventTimer, timerGetFrequency(eventTimer) * GROUP_RATE);
  timerSetCallback(eventTimer, onEvent, &timerEvent);
  timerEnable(eventTimer);

  struct Interface * const can = boardSetupCan(chronoTimer);
  ifSetCallback(can, onEvent, &canEvent);
  ifSetParam(can, IF_CAN_ACTIVE, NULL);

  while (1)
  {
    while (!canEvent && !timerEvent)
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

      pinToggle(led);
      ifRead(can, &message, sizeof(message));
      pinToggle(led);
    }
  }

  return 0;
}