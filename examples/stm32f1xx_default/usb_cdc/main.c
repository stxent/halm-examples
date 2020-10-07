/*
 * stm32f1xx_default/usb_cdc/main.c
 * Copyright (C) 2018 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/stm/stm32f1xx/clocking.h>
#include <halm/platform/stm/usb_device.h>
#include <halm/usb/cdc_acm.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_SIZE 64
#define LED_PIN     PIN(PORT_C, 13)
/*----------------------------------------------------------------------------*/
static const struct UsbDeviceConfig usbConfig = {
    .dm = PIN(PORT_A, 11),
    .dp = PIN(PORT_A, 12),
    .vid = 0x15A2,
    .pid = 0x0044,
    .channel = 0
};

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 8000000
};

static const struct MainPllConfig mainPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 1,
    .multiplier = 9
};

static const struct SystemClockConfig systemClockConfig = {
    .source = CLOCK_PLL
};

static const struct BusClockConfig ahbBusClockConfig = {
    .divisor = 1
};

static const struct BusClockConfig apbBusClockConfig = {
    .divisor = 2
};
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainPll, &mainPllConfig);
  while (!clockReady(MainPll));

  clockEnable(Apb1Clock, &apbBusClockConfig);
  clockEnable(Apb2Clock, &apbBusClockConfig);
  clockEnable(SystemClock, &systemClockConfig);

  clockEnable(MainClock, &ahbBusClockConfig);
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
