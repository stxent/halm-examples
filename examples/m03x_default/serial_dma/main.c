/*
 * m03x_default/serial_dma/main.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/numicro/clocking.h>
#include <halm/platform/numicro/serial_dma.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_LENGTH 128
#define LED_PIN       PIN(PORT_B, 14)
/*----------------------------------------------------------------------------*/
static const struct SerialDmaConfig serialConfig = {
    .rxChunks = 8,
    .rxLength = BUFFER_LENGTH,
    .txLength = BUFFER_LENGTH,
    .rate = 19200,
    .rx = PIN(PORT_A, 0),
    .tx = PIN(PORT_A, 1),
    .channel = 0,
    .dma = {DMA0_CHANNEL0, DMA0_CHANNEL1}
};
/*----------------------------------------------------------------------------*/
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

static const struct ExtendedClockConfig uartClockConfig = {
    .source = CLOCK_PLL,
    .divisor = 2
};
/*----------------------------------------------------------------------------*/
static void onSerialEvent(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
static void setupClock(void)
{
  const void * const UART_CLOCKS[] = {
      Uart0Clock, Uart1Clock, Uart2Clock, Uart3Clock,
      Uart4Clock, Uart5Clock, Uart6Clock, Uart7Clock
  };

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(UART_CLOCKS[serialConfig.channel], &uartClockConfig);
  clockEnable(MainClock, &mainClockConfig);
}
/*----------------------------------------------------------------------------*/
static void transferData(struct Interface *interface, struct Pin led)
{
  size_t available = 0;

  pinSet(led);

  do
  {
    uint8_t buffer[BUFFER_LENGTH];
    const size_t length = ifRead(interface, buffer, sizeof(buffer));

    ifWrite(interface, buffer, length);
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

  bool event = false;

  struct Interface * const serial = init(SerialDma, &serialConfig);
  assert(serial);
  ifSetCallback(serial, onSerialEvent, &event);

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    transferData(serial, led);
  }

  return 0;
}
