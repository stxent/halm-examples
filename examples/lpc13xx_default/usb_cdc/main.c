/*
 * lpc13xx_default/usb_cdc/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/usb/cdc_acm.h>
#include <halm/usb/usb.h>
#include <xcore/memory.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
static void onSerialEvent(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
static void transferData(struct Interface *interface)
{
  uint8_t buffer[BOARD_UART_BUFFER];
  size_t length;

  while ((length = ifRead(interface, buffer, sizeof(buffer))))
  {
    const uint8_t *position = buffer;

    while (length)
    {
      const size_t written = ifWrite(interface, position, length);

      length -= written;
      position += written;
    }
  }
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  bool event = false;

  boardSetupClockPll();

  const struct Pin dataLed = pinInit(BOARD_LED_0);
  pinOutput(dataLed, BOARD_LED_INV);
  const struct Pin workLed = pinInit(BOARD_LED_1);
  pinOutput(workLed, BOARD_LED_INV);

  struct Entity * const usb = boardSetupUsb();

  const struct CdcAcmConfig config = {
      .device = usb,
      .arena = NULL,
      .rxBuffers = 4,
      .txBuffers = 4,

      .endpoints = {
          .interrupt = 0x81,
          .rx = 0x03,
          .tx = 0x83
      }
  };

  struct Interface * const serial = init(CdcAcm, &config);
  assert(serial != NULL);
  ifSetCallback(serial, onSerialEvent, &event);

  usbDevSetConnected(usb, true);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    size_t available;
    uint8_t status;

    if (ifGetParam(serial, IF_CDC_ACM_STATUS, &status) == E_OK)
    {
      if (status & CDC_ACM_SUSPENDED)
        pinWrite(workLed, BOARD_LED_INV);
      else
        pinWrite(workLed, !BOARD_LED_INV);
    }

    if (ifGetParam(serial, IF_RX_AVAILABLE, &available) == E_OK && available)
    {
      transferData(serial);
      pinToggle(dataLed);
    }
  }

  return 0;
}
