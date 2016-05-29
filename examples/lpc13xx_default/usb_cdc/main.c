/*
 * main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <stdbool.h>

#include <memory.h>
#include <pin.h>
#include <platform/nxp/lpc13xx/clocking.h>
#include <platform/nxp/usb_device.h>
#include <usb/cdc_acm.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN PIN(3, 0)
/*----------------------------------------------------------------------------*/
static struct Entity *usb = 0;
static struct Interface *serial = 0;
/*----------------------------------------------------------------------------*/
static const struct UsbDeviceConfig usbConfig = {
    .dm = PIN(PORT_USB, PIN_USB_DM),
    .dp = PIN(PORT_USB, PIN_USB_DP),
    .connect = PIN(PORT_0, 6),
    .vbus = PIN(PORT_0, 3),
    .channel = 0,
    .priority = 0
};

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct PllConfig sysPllConfig = {
    .multiplier = 16,
    .divisor = 4,
    .source = CLOCK_EXTERNAL
};

static const struct PllConfig usbPllConfig = {
    .multiplier = 16,
    .divisor = 4,
    .source = CLOCK_EXTERNAL
};

static const struct CommonClockConfig mainClkConfig = {
    .source = CLOCK_PLL
};

static const struct CommonClockConfig usbClkConfig = {
    .source = CLOCK_USB_PLL
};
/*----------------------------------------------------------------------------*/
static void enableClock(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(UsbPll, &usbPllConfig);
  while (!clockReady(UsbPll));

  clockEnable(MainClock, &mainClkConfig);

  clockEnable(UsbClock, &usbClkConfig);
  while (!clockReady(UsbClock));
}
/*----------------------------------------------------------------------------*/
static void serialEventCallback(void *argument)
{
  bool * const eventPointer = argument;

  *eventPointer = true;
}
/*----------------------------------------------------------------------------*/
static void processInput(struct Interface *interface, const char *input,
    size_t length)
{
  char buffer[8];

  while (length)
  {
    const size_t chunkLength = length > sizeof(buffer) ?
        sizeof(buffer) : length;

    for (size_t index = 0; index < chunkLength; ++index)
      buffer[index] = input[index] + 1;

    ifWrite(interface, buffer, chunkLength);
    length -= chunkLength;
    input += chunkLength;
  }
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  char buffer[64];
  bool event = false;
  struct Pin led;

  led = pinInit(LED_PIN);
  pinOutput(led, 0);

  enableClock();

  usb = init(UsbDevice, &usbConfig);
  assert(usb);

  const struct CdcAcmConfig config = {
      .device = usb,
      .rxBuffers = 4,
      .txBuffers = 4,

      .endpoint = {
          .interrupt = 0x82,
          .rx = 0x03,
          .tx = 0x83
      }
  };

  serial = init(CdcAcm, &config);
  ifCallback(serial, serialEventCallback, &event);

  while (1)
  {
    while (!event)
      barrier();

    event = false;

    size_t available;

    if (ifGet(serial, IF_AVAILABLE, &available) == E_OK && available > 0)
    {
      pinSet(led);

      while (available)
      {
        const size_t bytesRead = ifRead(serial, buffer, sizeof(buffer));

        processInput(serial, buffer, bytesRead);
        available -= bytesRead;
      }

      pinReset(led);
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
