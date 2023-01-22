/*
 * lpc17xx_default/serial/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <xcore/interface.h>
#include <xcore/memory.h>
/*----------------------------------------------------------------------------*/
static void onSerialEvent(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
static void transferData(struct Interface *interface, struct Pin led)
{
  size_t available = 0;

  pinSet(led);

  do
  {
    uint8_t buffer[BOARD_UART_BUFFER];
    const size_t length = ifRead(interface, buffer, sizeof(buffer));

    ifWrite(interface, buffer, length);
    ifGetParam(interface, IF_RX_AVAILABLE, &available);
  }
  while (available > 0);

  pinReset(led);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  bool event = false;

  boardSetupClockPll();

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, false);

  struct Interface * const serial = boardSetupSerial();
  ifSetCallback(serial, onSerialEvent, &event);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    transferData(serial, led);
  }

  return 0;
}
