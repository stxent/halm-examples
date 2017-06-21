/*
 * lpc17xx_default/usb_cdc_composite/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/pin.h>
#include <halm/platform/nxp/usb_device.h>
#include <halm/platform/nxp/lpc17xx/clocking.h>
#include <halm/usb/cdc_acm.h>
#include <halm/usb/composite_device.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_SIZE  64
#define STREAM_COUNT 2
/*----------------------------------------------------------------------------*/
struct StreamDescriptor
{
  struct Interface *interface;
  struct Pin indication;
  bool event;
};
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct PllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 4,
    .multiplier = 32
};

static const struct CommonClockConfig pllClockSource = {
    .source = CLOCK_PLL
};
/*----------------------------------------------------------------------------*/
static const struct UsbDeviceConfig usbConfig = {
    .dm = PIN(0, 30),
    .dp = PIN(0, 29),
    .connect = PIN(2, 9),
    .vbus = PIN(1, 30),
    .vid = 0x15A2,
    .pid = 0x0044,
    .channel = 0
};
/*----------------------------------------------------------------------------*/
static void setupClock()
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &pllClockSource);

  clockEnable(UsbClock, &pllClockSource);
  while (!clockReady(UsbClock));
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
      pinInit(PIN(1, 8)),
      pinInit(PIN(1, 9))
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

    streams[i].interface = init(CdcAcm, &config);
    assert(streams[i].interface);
    ifSetCallback(streams[i].interface, onSerialEvent, streams + i);
    streams[i].event = false;
    streams[i].indication = leds[i];
    pinOutput(streams[i].indication, false);
  }
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
      barrier();
      for (size_t i = 0; i < STREAM_COUNT; ++i)
        event = event || streams[i].event;
    }
    while (!event);

    for (size_t i = 0; i < STREAM_COUNT; ++i)
    {
      if (!streams[i].event)
        continue;

      streams[i].event = false;
      pinSet(streams[i].indication);

      size_t read;
      char buffer[BUFFER_SIZE];

      while ((read = ifRead(streams[i].interface, buffer, sizeof(buffer))))
      {
        size_t pending = read;
        const char *bufferPointer = buffer;

        while (pending)
        {
          const size_t written = ifWrite(streams[i].interface, buffer, pending);

          pending -= written;
          bufferPointer += written;
        }
      }

      pinReset(streams[i].indication);
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
