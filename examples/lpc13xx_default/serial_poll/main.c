/*
 * lpc13xx_default/serial_poll/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/nxp/serial_poll.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_SIZE 16
#define LED_PIN     PIN(3, 0)
/*----------------------------------------------------------------------------*/
static const struct SerialPollConfig serialConfig = {
    .rate = 19200,
    .rx = PIN(1, 6),
    .tx = PIN(1, 7),
    .channel = 0
};
/*----------------------------------------------------------------------------*/
int main(void)
{
  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  struct Interface * const serial = init(SerialPoll, &serialConfig);
  assert(serial);

  char buffer[BUFFER_SIZE];

  while (1)
  {
    const size_t bytesRead = ifRead(serial, buffer, sizeof(buffer));

    if (bytesRead)
    {
      pinSet(led);
      ifWrite(serial, buffer, bytesRead);
      pinReset(led);
    }
  }

  return 0;
}
