/*
 * main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <stdbool.h>

#include <memory.h>
#include <pin.h>
#include <platform/nxp/lpc43xx/clocking.h>
#include <platform/nxp/usb_device.h>
#include <usb/cdc_acm.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN PIN(PORT_6, 6)
/*----------------------------------------------------------------------------*/
static struct Entity *usb = 0;
static struct Interface *serial = 0;
/*----------------------------------------------------------------------------*/
static const struct UsbDeviceConfig usbConfig = {
    .dm = PIN(PORT_USB, PIN_USB0_DM),
    .dp = PIN(PORT_USB, PIN_USB0_DP),
    .connect = 0,
    .vbus = PIN(PORT_USB, PIN_USB0_VBUS),
    .channel = 0,
    .priority = 0
};

static const struct PllConfig pllConfig = {
    .multiplier = 20,
    .divisor = 4,
    .source = CLOCK_EXTERNAL
};

static const struct PllConfig usbPllConfig = {
    .multiplier = 40,
    .divisor = 1,
    .source = CLOCK_EXTERNAL
};

static const struct CommonClockConfig initialClock = {
    .source = CLOCK_INTERNAL
};

static const struct CommonClockConfig baseClock = {
    .source = CLOCK_PLL
};

static const struct ExternalOscConfig externalClock = {
    .frequency = 12000000,
    .bypass = false
};
/*----------------------------------------------------------------------------*/
static void enableClock(void)
{
  clockEnable(MainClock, &initialClock);
  while (!clockReady(MainClock));

  clockEnable(ExternalOsc, &externalClock);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &pllConfig);
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &baseClock);
  while (!clockReady(MainClock));

  clockEnable(UsbPll, &usbPllConfig);
  while (!clockReady(UsbPll));
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
  char buffer[512];
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
          .interrupt = 0x81,
          .rx = 0x02,
          .tx = 0x82
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
