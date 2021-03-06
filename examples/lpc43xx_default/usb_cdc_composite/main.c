/*
 * lpc43xx_default/usb_cdc_composite/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/usb_device.h>
#include <halm/usb/cdc_acm.h>
#include <halm/usb/composite_device.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_LENGTH 512
#define STREAM_COUNT  2
/*----------------------------------------------------------------------------*/
struct StreamDescriptor
{
  struct Interface *stream;
  struct Pin led;
  bool event;
};
/*----------------------------------------------------------------------------*/
static const struct GenericClockConfig initialClockConfig = {
    .source = CLOCK_INTERNAL
};

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_PLL
};

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000,
    .bypass = false
};

static const struct PllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 4,
    .multiplier = 20
};

static const struct PllConfig usbPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 1,
    .multiplier = 40
};
/*----------------------------------------------------------------------------*/
static const struct UsbDeviceConfig usbConfig = {
    .dm = PIN(PORT_USB, PIN_USB0_DM),
    .dp = PIN(PORT_USB, PIN_USB0_DP),
    .connect = 0,
    .vbus = PIN(PORT_USB, PIN_USB0_VBUS),
    .vid = 0x15A2,
    .pid = 0x0044,
    .channel = 0
};
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(MainClock, &initialClockConfig);

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(UsbPll, &usbPllConfig);
  while (!clockReady(UsbPll));

  clockEnable(MainClock, &mainClockConfig);
}
/*----------------------------------------------------------------------------*/
static void onSerialEvent(void *argument)
{
  struct StreamDescriptor * const descriptor = argument;

  descriptor->event = true;
}
/*----------------------------------------------------------------------------*/
static void initStreams(struct StreamDescriptor *streams, void *parent)
{
  enum {
    EP_INT,
    EP_RX,
    EP_TX
  };

  static const uint8_t endpointAddressMap[][3] = {
      {
          [EP_INT] = 0x81,
          [EP_RX]  = 0x02,
          [EP_TX]  = 0x82
      },
      {
          [EP_INT] = 0x83,
          [EP_RX]  = 0x04,
          [EP_TX]  = 0x84
      }
  };
  static_assert(ARRAY_SIZE(endpointAddressMap) == STREAM_COUNT, "Error");

  const struct Pin leds[] = {
      pinInit(PIN(PORT_6, 6)),
      pinInit(PIN(PORT_6, 7))
  };
  static_assert(ARRAY_SIZE(leds) == STREAM_COUNT, "Error");

  struct CdcAcmConfig config = {
      .device = parent,
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
    uint8_t buffer[BUFFER_LENGTH];
    uint8_t *position = buffer;
    size_t length = ifRead(interface, buffer, sizeof(buffer));

    while (length)
    {
      const size_t written = ifWrite(interface, position, length);

      length -= written;
      position += written;
    }

    ifGetParam(interface, IF_AVAILABLE, &available);
  }
  while (available > 0);

  pinReset(led);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  struct StreamDescriptor streams[STREAM_COUNT];

  setupClock();

  struct Entity * const usb = init(UsbDevice, &usbConfig);
  assert(usb);

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
