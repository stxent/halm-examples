/*
 * lpc43xx_default/usb_cdc/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/nxp/lpc43xx/clocking.h>
#include <halm/platform/nxp/usb_device.h>
#include <halm/usb/cdc_acm.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_SIZE 512
#define LED_PIN     PIN(PORT_6, 9)
#define USB_PORT    0
/*----------------------------------------------------------------------------*/
static const struct UsbDeviceConfig usbConfig[] = {
    {
        .dm = PIN(PORT_USB, PIN_USB0_DM),
        .dp = PIN(PORT_USB, PIN_USB0_DP),
        .connect = 0,
        .vbus = PIN(PORT_USB, PIN_USB0_VBUS),
        .vid = 0x15A2,
        .pid = 0x0044,
        .channel = 0
    }, {
        .dm = PIN(PORT_USB, PIN_USB1_DM),
        .dp = PIN(PORT_USB, PIN_USB1_DP),
        .connect = 0,
        .vbus = PIN(PORT_2, 5),
        .vid = 0x15A2,
        .pid = 0x0044,
        .channel = 1
    }
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
static void setupClock(void)
{
  clockEnable(MainClock, &initialClockConfig);

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &mainClockConfig);

  clockEnable(UsbPll, &usbPllConfig);
  while (!clockReady(UsbPll));

  clockEnable(Usb1Clock, &mainClockConfig);
  while (!clockReady(UsbPll));
}
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
  setupClock();

  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  struct Entity * const usb = init(UsbDevice, &usbConfig[USB_PORT]);
  assert(usb);

  const struct CdcAcmConfig config = {
      .device = usb,
      .rxBuffers = 4,
      .txBuffers = 4,

      .endpoints = {
          .interrupt = 0x81,
          .rx = 0x02,
          .tx = 0x82
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
