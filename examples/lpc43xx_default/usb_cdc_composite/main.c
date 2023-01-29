/*
 * lpc43xx_default/usb_cdc_composite/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/usb/cdc_acm.h>
#include <halm/usb/composite_device.h>
#include <xcore/memory.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define STREAM_COUNT 2
/*----------------------------------------------------------------------------*/
struct StreamDescriptor
{
  struct Interface *stream;
  struct Pin led;
  bool event;
};
/*----------------------------------------------------------------------------*/
static void onSerialEvent(void *argument)
{
  struct StreamDescriptor * const descriptor = argument;
  descriptor->event = true;
}
/*----------------------------------------------------------------------------*/
static void initStreams(struct StreamDescriptor *streams, void *parent)
{
  enum
  {
    EP_INT,
    EP_RX,
    EP_TX
  };

  static const uint8_t endpointAddressMap[][3] = {
      {
          [EP_INT] = 0x81,
          [EP_RX]  = 0x02,
          [EP_TX]  = 0x82
      }, {
          [EP_INT] = 0x83,
          [EP_RX]  = 0x04,
          [EP_TX]  = 0x84
      }
  };
  static_assert(ARRAY_SIZE(endpointAddressMap) == STREAM_COUNT, "Error");

  const struct Pin leds[] = {
      pinInit(BOARD_LED_0),
      pinInit(BOARD_LED_1)
  };
  static_assert(ARRAY_SIZE(leds) == STREAM_COUNT, "Error");

  struct CdcAcmConfig config = {
      .device = parent,
      .arena = 0,
      .rxBuffers = 4,
      .txBuffers = 4
  };

  for (size_t i = 0; i < STREAM_COUNT; ++i)
  {
    config.endpoints.interrupt = endpointAddressMap[i][EP_INT];
    config.endpoints.rx = endpointAddressMap[i][EP_RX];
    config.endpoints.tx = endpointAddressMap[i][EP_TX];

    streams[i].stream = init(CdcAcm, &config);
    assert(streams[i].stream);
    ifSetCallback(streams[i].stream, onSerialEvent, streams + i);
    streams[i].event = false;
    streams[i].led = leds[i];
    pinOutput(streams[i].led, false);
  }
}
/*----------------------------------------------------------------------------*/
static void transferData(struct Interface *interface, struct Pin led)
{
  size_t available = 0;

  pinSet(led);

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

  pinReset(led);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  static const bool USE_HS_USB = true;
  struct StreamDescriptor streams[STREAM_COUNT];

  boardSetupClockPll();

  struct Entity * const usb = USE_HS_USB ? boardSetupUsb0() : boardSetupUsb1();

  const struct CompositeDeviceConfig compositeConfig = {
      .device = usb
  };
  struct Entity * const composite = init(CompositeDevice, &compositeConfig);
  assert(composite);

  initStreams(streams, composite);
  usbDevSetConnected(composite, true);

  while (1)
  {
    bool event = false;

    do
    {
      for (size_t i = 0; i < STREAM_COUNT; ++i)
        event = event || streams[i].event;
      barrier();
    }
    while (!event);

    for (size_t i = 0; i < STREAM_COUNT; ++i)
    {
      if (!streams[i].event)
        continue;
      streams[i].event = false;

      transferData(streams[i].stream, streams[i].led);
    }
  }

  return 0;
}
