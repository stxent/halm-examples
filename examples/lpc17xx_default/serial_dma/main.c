/*
 * lpc17xx_default/serial_dma/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include <halm/pin.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/gptimer.h>
#include <halm/platform/lpc/serial_dma.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
#define BUFFER_LENGTH 32
#define LED_PIN       PIN(1, 8)
#define UART_CHANNEL  1
#define UART_RATE     19200
#define USE_TIMER
/*----------------------------------------------------------------------------*/
static const struct SerialDmaConfig serialConfig[] = {
    /* UART0 */
    {
        .rate = UART_RATE,
        .rxChunks = 8,
        .rxLength = BUFFER_LENGTH * 8,
        .txLength = BUFFER_LENGTH * 8,
        .rx = PIN(0, 3),
        .tx = PIN(0, 2),
        .channel = 0,
        .dma = {0, 1}
    },
    /* UART1 */
    {
        .rate = UART_RATE,
        .rxChunks = 8,
        .rxLength = BUFFER_LENGTH * 8,
        .txLength = BUFFER_LENGTH * 8,
        .rx = PIN(0, 16),
        .tx = PIN(0, 15),
        .channel = 1,
        .dma = {3, 2}
    },
    /* UART2 */
    {
        .rate = UART_RATE,
        .rxChunks = 8,
        .rxLength = BUFFER_LENGTH * 8,
        .txLength = BUFFER_LENGTH * 8,
        .rx = PIN(0, 11),
        .tx = PIN(0, 10),
        .channel = 2,
        .dma = {0, 1}
    },
    /* UART3 */
    {
        .rate = UART_RATE,
        .rxChunks = 8,
        .rxLength = BUFFER_LENGTH * 8,
        .txLength = BUFFER_LENGTH * 8,
        .rx = PIN(4, 29),
        .tx = PIN(4, 28),
        .channel = 3,
        .dma = {7, 6}
    }
};

static const struct GpTimerConfig timerConfig = {
    .frequency = 1000,
    .channel = 0
};
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct PllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 16,
    .multiplier = 32
};

static const struct GenericClockConfig mainClockConfig = {
    .source = CLOCK_PLL
};
/*----------------------------------------------------------------------------*/
static void onSerialEvent(void *argument)
{
  *(bool *)argument = true;
}
/*----------------------------------------------------------------------------*/
static void onTimerOverflow(void *argument)
{
  *(bool *)argument = true;
}
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

  struct Interface * const serial = init(SerialDma,
      &serialConfig[UART_CHANNEL]);
  assert(serial);
  ifSetCallback(serial, onSerialEvent, &event);

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  timerSetOverflow(timer, 100);
  timerSetCallback(timer, onTimerOverflow, &event);

#ifdef USE_TIMER
  timerEnable(timer);
#endif

  while (1)
  {
    while (!event)
      barrier();
    event = false;

    transferData(serial, led);
  }

  return 0;
}
