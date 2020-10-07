/*
 * lpc13uxx_default/serial/main.c
 * Copyright (C) 2020 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/nxp/serial.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_SIZE 64
#define LED_PIN     PIN(1, 13)
/*----------------------------------------------------------------------------*/
static const struct SerialConfig serialConfig = {
    .rate = 19200,
    .rxLength = 64,
    .txLength = 64,
    .rx = PIN(0, 18),
    .tx = PIN(0, 19),
    .channel = 0
};
/*----------------------------------------------------------------------------*/
static void onSerialEvent(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
static void processInput(struct Interface *interface, char *buffer,
    size_t length)
{
  for (size_t index = 0; index < length; ++index)
    ++buffer[index];

  while (length)
  {
    const size_t bytesWritten = ifWrite(interface, buffer, length);

    length -= bytesWritten;
    buffer += bytesWritten;
  }
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  bool event = false;

  struct Interface * const serial = init(Serial, &serialConfig);
  assert(serial);
  ifSetCallback(serial, onSerialEvent, &event);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    size_t available;

    if (ifGetParam(serial, IF_AVAILABLE, &available) == E_OK && available > 0)
    {
      char buffer[BUFFER_SIZE];
      size_t bytesRead;

      pinSet(led);

      while ((bytesRead = ifRead(serial, buffer, sizeof(buffer))))
        processInput(serial, buffer, bytesRead);

      pinReset(led);
    }
  }

  return 0;
}
