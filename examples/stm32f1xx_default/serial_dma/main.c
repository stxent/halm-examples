/*
 * stm32f1xx_default/serial_dma/main.c
 * Copyright (C) 2020 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/stm32/serial_dma.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_LENGTH 64
#define LED_PIN       PIN(PORT_C, 13)
/*----------------------------------------------------------------------------*/
static const struct SerialDmaConfig serialConfig = {
    .rate = 19200,
    .rxChunk = BUFFER_LENGTH / 4,
    .rxLength = BUFFER_LENGTH,
    .txLength = BUFFER_LENGTH,
    .rx = PIN(PORT_B, 7),
    .tx = PIN(PORT_B, 6),
    .channel = 0,
    .rxDma = DMA1_STREAM5,
    .txDma = DMA1_STREAM4
};
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
    uint8_t buffer[BUFFER_LENGTH];
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
  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  bool event = false;

  struct Interface * const serial = init(SerialDma, &serialConfig);
  assert(serial);
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
