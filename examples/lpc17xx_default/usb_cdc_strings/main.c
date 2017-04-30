/*
 * main.c
 * Copyright (C) 2017 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/pin.h>
#include <halm/platform/nxp/lpc17xx/clocking.h>
#include <halm/platform/nxp/usb_device.h>
#include <halm/usb/cdc_acm.h>
#include <halm/usb/usb_langid.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_SIZE 64
#define LED_PIN     PIN(1, 8)
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
static const char vendorStringEn[] = "Undefined";
static const char productStringEn[] = "LPC17xx DevBoard";
static const char serialStringEn[] = "00000001";
/*----------------------------------------------------------------------------*/
static void customStringHeader(void *argument __attribute__((unused)),
    enum UsbLangId langid __attribute__((unused)),
    struct UsbDescriptor *header, void *payload)
{
  usbStringHeader(header, payload, LANGID_ENGLISH_US);
}
/*----------------------------------------------------------------------------*/
static void customStringWrapper(void *argument,
    enum UsbLangId langid __attribute__((unused)),
    struct UsbDescriptor *header, void *payload)
{
  usbStringWrap(header, payload, argument);
}
/*----------------------------------------------------------------------------*/
static void setupClock(void)
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
  *(bool *)argument = true;
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

  char buffer[BUFFER_SIZE];
  bool event = false;

  struct Interface * const serial = init(CdcAcm, &config);
  assert(serial);
  ifCallback(serial, onSerialEvent, &event);

  usbDevStringAppend(usb, usbStringBuild(customStringHeader, 0,
      USB_STRING_HEADER));
  usbDevStringAppend(usb, usbStringBuild(customStringWrapper,
      (void *)vendorStringEn, USB_STRING_VENDOR));
  usbDevStringAppend(usb, usbStringBuild(customStringWrapper,
      (void *)productStringEn, USB_STRING_PRODUCT));
  usbDevStringAppend(usb, usbStringBuild(customStringWrapper,
      (void *)serialStringEn, USB_STRING_SERIAL));

  usbDevSetConnected(usb, true);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    size_t bytesRead;

    pinSet(led);
    while ((bytesRead = ifRead(serial, buffer, sizeof(buffer))))
      ifWrite(serial, buffer, bytesRead);
    pinReset(led);
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
