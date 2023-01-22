/*
 * m03x_default/usb_cdc/main.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/numicro/clocking.h>
#include <halm/platform/numicro/usb_device.h>
#include <halm/usb/cdc_acm.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_LENGTH 64
#define LED_PIN       PIN(PORT_B, 14)
/*----------------------------------------------------------------------------*/
static const struct UsbDeviceConfig usbConfig = {
    .dm = PIN(PORT_USB, PIN_USB_DM),
    .dp = PIN(PORT_USB, PIN_USB_DP),
    .vbus = PIN(PORT_USB, PIN_USB_VBUS),
    .vid = 0x15A2,
    .pid = 0x0044,
    .channel = 0
};

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 32000000
};

static const struct ExtendedClockConfig mainClockConfig = {
    .source = CLOCK_PLL,
    .divisor = 2
};

static const struct PllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 4,
    .multiplier = 12
};

static const struct ExtendedClockConfig usbClockConfig = {
    .source = CLOCK_PLL,
    .divisor = 2
};
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(UsbClock, &usbClockConfig);
  clockEnable(MainClock, &mainClockConfig);
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

    ifGetParam(interface, IF_RX_AVAILABLE, &available);
  }
  while (available > 0);

  pinReset(led);
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  struct Entity * const usb = init(UsbDevice, &usbConfig);
  assert(usb);

  const struct CdcAcmConfig config = {
      .device = usb,
      .arena = 0,
      .rxBuffers = 4,
      .txBuffers = 4,

      .endpoints = {
          .interrupt = 0x81,
          .rx = 0x03,
          .tx = 0x83
      }
  };

  bool event = false;

  struct Interface * const serial = init(CdcAcm, &config);
  assert(serial);
  ifSetCallback(serial, onSerialEvent, &event);

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
