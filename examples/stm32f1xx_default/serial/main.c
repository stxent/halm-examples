/*
 * stm32f1xx_default/serial/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/pin.h>
#include <halm/platform/stm/serial.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_SIZE 64
#define LED_PIN     PIN(PORT_C, 14)
/*----------------------------------------------------------------------------*/
static const struct SerialConfig serialConfig = {
    .rate = 19200,
    .rxLength = 64,
    .txLength = 64,
    .rx = PIN(PORT_B, 11),
    .tx = PIN(PORT_B, 10),
    .channel = 2
};
/*----------------------------------------------------------------------------*/
static void onSerialEvent(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
static void processInput(struct Interface *interface, const char *input,
    size_t length)
{
  char buffer[16];

  while (length)
  {
    const size_t chunkLength = length > sizeof(buffer) ?
        sizeof(buffer) : length;

    for (size_t index = 0; index < chunkLength; ++index)
      buffer[index] = input[index] + 1;

    size_t pending = chunkLength;
    const char *bufferPointer = buffer;

    while (pending)
    {
      const size_t bytesWritten = ifWrite(interface, buffer, chunkLength);

      pending -= bytesWritten;
      bufferPointer += bytesWritten;
    }

    length -= chunkLength;
    input += chunkLength;
  }
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  char buffer[BUFFER_SIZE];
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
      size_t bytesRead;

      pinSet(led);

      while ((bytesRead = ifRead(serial, buffer, sizeof(buffer))))
        processInput(serial, buffer, bytesRead);

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
