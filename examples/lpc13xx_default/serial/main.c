/*
 * main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <stdbool.h>

#include <memory.h>
#include <pin.h>
#include <platform/nxp/serial.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN PIN(3, 0)
/*----------------------------------------------------------------------------*/
static struct Interface *serial = 0;
/*----------------------------------------------------------------------------*/
static const struct SerialConfig serialConfig = {
    .channel = 0,
    .rx = PIN(1, 6),
    .tx = PIN(1, 7),
    .rate = 19200,
    .rxLength = 64,
    .txLength = 64
};
/*----------------------------------------------------------------------------*/
static void serialEventCallback(void *argument)
{
  bool * const eventPointer = argument;

  *eventPointer = true;
}
/*----------------------------------------------------------------------------*/
static void processInput(struct Interface *interface, const char *input,
    size_t length)
{
  char buffer[8];

  while (length)
  {
    const size_t chunkLength = length > sizeof(buffer) ?
        sizeof(buffer) : length;

    for (size_t index = 0; index < chunkLength; ++index)
      buffer[index] = input[index] + 1;

    ifWrite(interface, buffer, chunkLength);
    length -= chunkLength;
    input += chunkLength;
  }
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  bool event = false;
  struct Pin led;

  led = pinInit(LED_PIN);
  pinOutput(led, 0);

  serial = init(Serial, &serialConfig);
  assert(serial);
  ifCallback(serial, serialEventCallback, &event);

  while (1)
  {
    while (!event)
      barrier();

    event = false;

    size_t available;

    if (ifGet(serial, IF_AVAILABLE, &available) == E_OK && available > 0)
    {
      char buffer[16];

      pinSet(led);

      while (available)
      {
        const size_t bytesRead = ifRead(serial, buffer, sizeof(buffer));

        processInput(serial, buffer, bytesRead);
        available -= bytesRead;
      }

      pinReset(led);
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
