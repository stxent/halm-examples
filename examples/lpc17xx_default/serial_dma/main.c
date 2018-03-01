/*
 * lpc17xx_default/serial_dma/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <assert.h>
#include <halm/pin.h>
#include <halm/platform/nxp/serial_dma.h>
#include <halm/platform/nxp/lpc17xx/clocking.h>
/*----------------------------------------------------------------------------*/
#define LED_PIN       PIN(1, 8)
#define UART_CHANNEL  0
/*----------------------------------------------------------------------------*/
static const struct SerialDmaConfig serialConfig[] = {
    /* UART0 */
    {
        .rate = 19200,
        .rxChunk = 16,
        .rxLength = 128,
        .txLength = 128,
        .rx = PIN(0, 3),
        .tx = PIN(0, 2),
        .channel = 0,
        .dma = {0, 1}
    },
    /* UART1 */
    {
        .rate = 19200,
        .rxChunk = 16,
        .rxLength = 128,
        .txLength = 128,
        .rx = PIN(0, 16),
        .tx = PIN(0, 15),
        .channel = 1,
        .dma = {3, 2}
    },
    /* UART2 */
    {
        .rate = 19200,
        .rxChunk = 16,
        .rxLength = 128,
        .txLength = 128,
        .rx = PIN(0, 11),
        .tx = PIN(0, 10),
        .channel = 2,
        .dma = {0, 1}
    },
    /* UART3 */
    {
        .rate = 19200,
        .rxChunk = 16,
        .rxLength = 128,
        .txLength = 128,
        .rx = PIN(4, 29),
        .tx = PIN(4, 28),
        .channel = 3,
        .dma = {7, 6}
    }
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

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_PLL
};
/*----------------------------------------------------------------------------*/
static uint8_t buffer[128];
static bool event = false;
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &mainClockConfig);
}
/*----------------------------------------------------------------------------*/
static void onSerialEvent(void *argument)
{
  struct Interface * const interface = argument;
  size_t available;

  if (ifGetParam(interface, IF_AVAILABLE, &available) == E_OK && available)
    event = true;
}
/*----------------------------------------------------------------------------*/
int main(void)
{
  setupClock();

  const struct Pin led = pinInit(LED_PIN);
  pinOutput(led, false);

  struct Interface * const serial =
      init(SerialDma, &serialConfig[UART_CHANNEL]);
  assert(serial);
  ifSetParam(serial, IF_ZEROCOPY, 0);
  ifSetCallback(serial, onSerialEvent, serial);

  size_t readTotal = 0;
  size_t writtenTotal = 0;

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    size_t bytesRead;

    do
    {
      bytesRead = ifRead(serial, buffer, sizeof(buffer));
      readTotal += bytesRead;
      writtenTotal += ifWrite(serial, buffer, bytesRead);
    }
    while (bytesRead);

    /* Suppress warnings */
    (void)readTotal;
    (void)writtenTotal;
  }

  return 0;
}
