/*
 * lpc17xx_default/serial/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/generic/serial.h>
#include <xcore/memory.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
static void onSerialEvent(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
static void transferData(struct Interface *interface, struct Pin led)
{
  size_t available = 0;

  pinToggle(led);

  do
  {
    uint8_t buffer[BOARD_UART_BUFFER];
    const size_t length = ifRead(interface, buffer, sizeof(buffer));

    ifWrite(interface, buffer, length);
    ifGetParam(interface, IF_RX_AVAILABLE, &available);
  }
  while (available > 0);

  pinToggle(led);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  static const uint32_t UART_TEST_RATE = 19200;
  static const uint8_t UART_TEST_PARITY = SERIAL_PARITY_NONE;

  bool event = false;
  enum Result res;

  boardSetupClockPll();

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, BOARD_LED_INV);

  struct Interface * const serial = boardSetupSerial();
  ifSetCallback(serial, onSerialEvent, &event);
  res = ifSetParam(serial, IF_RATE, &UART_TEST_RATE);
  assert(res == E_OK);
  res = ifSetParam(serial, IF_SERIAL_PARITY, &UART_TEST_PARITY);
  assert(res == E_OK);

  /* Suppress warning */
  (void)res;

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    transferData(serial, led);
  }

  return 0;
}
