/*
 * lpc17xx_default/usb_cdc_strings/main.c
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/usb/cdc_acm.h>
#include <halm/usb/usb.h>
#include <halm/usb/usb_langid.h>
#include <xcore/memory.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
static const char vendorStringEn[] = "Undefined";
static const char productStringEn[] = "LPC17xx DevBoard";
static const char serialStringEn[] = "00000001";
/*----------------------------------------------------------------------------*/
static void customStringHeader(const void *argument __attribute__((unused)),
    enum UsbLangId langid __attribute__((unused)),
    struct UsbDescriptor *header, void *payload)
{
  usbStringHeader(header, payload, LANGID_ENGLISH_US);
}
/*----------------------------------------------------------------------------*/
static void customStringWrapper(const void *argument,
    enum UsbLangId langid __attribute__((unused)),
    struct UsbDescriptor *header, void *payload)
{
  usbStringWrap(header, payload, argument);
}
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
    uint8_t *position = buffer;
    size_t length = ifRead(interface, buffer, sizeof(buffer));

    while (length)
    {
      const size_t written = ifWrite(interface, position, length);

      length -= written;
      position += written;
    }

    ifGetParam(interface, IF_RX_AVAILABLE, &available);
  }
  while (available > 0);

  pinToggle(led);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  bool event = false;

  boardSetupClockPll();

  const struct Pin led = pinInit(BOARD_LED);
  pinOutput(led, BOARD_LED_INV);

  struct Entity * const usb = boardSetupUsb();

  const struct CdcAcmConfig config = {
      .device = usb,
      .arena = 0,
      .rxBuffers = 4,
      .txBuffers = 4,

      .endpoints = {
          .interrupt = 0x81,
          .rx = 0x02,
          .tx = 0x82
      }
  };

  struct Interface * const serial = init(CdcAcm, &config);
  assert(serial);
  ifSetCallback(serial, onSerialEvent, &event);

  usbDevStringAppend(usb, usbStringBuild(customStringHeader, 0,
      USB_STRING_HEADER, 0));
  usbDevStringAppend(usb, usbStringBuild(customStringWrapper,
      vendorStringEn, USB_STRING_VENDOR, 0));
  usbDevStringAppend(usb, usbStringBuild(customStringWrapper,
      productStringEn, USB_STRING_PRODUCT, 0));
  usbDevStringAppend(usb, usbStringBuild(customStringWrapper,
      serialStringEn, USB_STRING_SERIAL, 0));

  usbDevSetConnected(usb, true);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    transferData(serial, led);
  }

  return 0;
}
