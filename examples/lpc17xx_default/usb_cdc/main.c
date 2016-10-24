/*
 * main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/pin.h>
#include <halm/platform/nxp/lpc13xx/clocking.h>
#include <halm/platform/nxp/usb_device.h>
#include <halm/usb/cdc_acm.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_SIZE 64
#define LED_PIN     PIN(3, 0)
/*----------------------------------------------------------------------------*/
static const struct UsbDeviceConfig usbConfig = {
    .dm = PIN(0, 30),
    .dp = PIN(0, 29),
    .connect = PIN(2, 9),
    .vid = 0x15A2,
    .pid = 0x0044,
    .channel = 0
};

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct PllConfig sysPllConfig = {
    .multiplier = 32,
    .divisor = 4,
    .source = CLOCK_EXTERNAL
};

static const struct CommonClockConfig pllClockSource = {
    .source = CLOCK_PLL
};
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
static void processInput(struct Interface *interface, const char *input,
    size_t length)
{
  char buffer[16];

  while (length)
  {
    const size_t chunkLength = length > sizeof(buffer) ?
        sizeof(buffer) : length;

    for (size_t index = 0; index < chunkLength; ++index)
      buffer[index] = input[index] + 1;

    size_t pending = chunkLength;
    const char *bufferPointer = buffer;

    while (pending)
    {
      const size_t bytesWritten = ifWrite(interface, buffer, chunkLength);

      pending -= bytesWritten;
      bufferPointer += bytesWritten;
    }

    length -= chunkLength;
    input += chunkLength;
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

  char buffer[BUFFER_SIZE];
  bool event = false;

  struct Interface * const serial = init(CdcAcm, &config);
  assert(serial);
  ifCallback(serial, onSerialEvent, &event);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    size_t available;

    if (ifGet(serial, IF_AVAILABLE, &available) == E_OK && available > 0)
    {
      size_t bytesRead;

      pinSet(led);

      while ((bytesRead = ifRead(serial, buffer, sizeof(buffer))))
        processInput(serial, buffer, bytesRead);

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
