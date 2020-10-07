/*
 * lpc13uxx_default/usb_cdc/main.c
 * Copyright (C) 2020 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/nxp/lpc13uxx/clocking.h>
#include <halm/platform/nxp/usb_device.h>
#include <halm/usb/cdc_acm.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_SIZE 64
#define LED_PIN     PIN(1, 13)
/*----------------------------------------------------------------------------*/
static const struct UsbDeviceConfig usbConfig = {
    .dm = PIN(PORT_USB, PIN_USB_DM),
    .dp = PIN(PORT_USB, PIN_USB_DP),
    .connect = PIN(PORT_0, 6),
    .vbus = PIN(PORT_0, 3),
    .vid = 0x15A2,
    .pid = 0x0044,
    .channel = 0
};

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000,
    .bypass = false
};

static const struct PllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 4,
    .multiplier = 16
};

static const struct PllConfig usbPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 4,
    .multiplier = 16
};

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_PLL
};

static const struct GenericClockConfig usbClockConfig = {
    .source = CLOCK_MAIN
};
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(UsbPll, &usbPllConfig);
  while (!clockReady(UsbPll));

  clockEnable(MainClock, &mainClockConfig);

  /*
   * System PLL and USB PLL should be running to make both clock
   * sources available for switching. After the switch, the USB PLL
   * can be turned off.
   */
  clockEnable(UsbClock, &usbClockConfig);
  while (!clockReady(UsbClock));
  clockDisable(UsbPll);
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

  struct Entity * const usb = init(UsbDevice, &usbConfig);
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