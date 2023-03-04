/*
 * lpc13xx_default/serial_poll/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/generic/serial.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
int main(void)
{
  static const uint32_t UART_TEST_RATE = 19200;
  static const uint8_t UART_TEST_PARITY = SERIAL_PARITY_NONE;

  char buffer[BOARD_UART_BUFFER];
  enum Result res;

  boardSetupClockPll();

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, BOARD_LED_INV);

  struct Interface * const serial = boardSetupSerialPoll();
  res = ifSetParam(serial, IF_RATE, &UART_TEST_RATE);
  assert(res == E_OK);
  res = ifSetParam(serial, IF_SERIAL_PARITY, &UART_TEST_PARITY);
  assert(res == E_OK);

  /* Suppress warning */
  (void)res;

  while (1)
  {
    const size_t bytesRead = ifRead(serial, buffer, sizeof(buffer));

    if (bytesRead)
    {
      pinToggle(led);
      ifWrite(serial, buffer, bytesRead);
      pinToggle(led);
    }
  }

  return 0;
}
