/*
 * stm32f1xx_default/shared/board.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/stm32/can.h>
#include <halm/platform/stm32/clocking.h>
#include <halm/platform/stm32/exti.h>
#include <halm/platform/stm32/gptimer.h>
#include <halm/platform/stm32/serial.h>
#include <halm/platform/stm32/serial_dma.h>
#include <halm/platform/stm32/spi.h>
#include <halm/platform/stm32/usb_device.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
static const struct GpTimerConfig adcTimerConfig = {
    .frequency = 10000,
    .channel = TIM4,
    .event = 4
};

static const struct ExtiConfig buttonIntConfig = {
    .pin = BOARD_BUTTON,
    .event = PIN_FALLING,
    .pull = PIN_PULLUP
};

static struct CanConfig canConfig = {
    .timer = 0,
    .rate = 1000000,
    .rxBuffers = 4,
    .txBuffers = 4,
    .rx = PIN(PORT_B, 8),
    .tx = PIN(PORT_B, 9),
    .priority = 0,
    .channel = 0
};

static const struct SerialConfig serialConfig = {
    .rxLength = BOARD_UART_BUFFER,
    .txLength = BOARD_UART_BUFFER,
    .rate = 19200,
    .rx = PIN(PORT_B, 7),
    .tx = PIN(PORT_B, 6),
    .channel = USART1
};

static const struct SerialDmaConfig serialDmaConfig = {
    .rxChunk = BOARD_UART_BUFFER / 4,
    .rxLength = BOARD_UART_BUFFER,
    .txLength = BOARD_UART_BUFFER,
    .rate = 19200,
    .rx = PIN(PORT_B, 7),
    .tx = PIN(PORT_B, 6),
    .channel = USART1,
    .rxDma = DMA1_STREAM5,
    .txDma = DMA1_STREAM4
};

static const struct SpiConfig spiConfig = {
    .rate = 2000000,
    .miso = PIN(PORT_A, 6),
    .mosi = PIN(PORT_A, 7),
    .sck = PIN(PORT_A, 5),
    .channel = SPI1,
    .mode = 0,
    .rxDma = DMA1_STREAM2,
    .txDma = DMA1_STREAM3
};

static const struct GpTimerConfig timerConfig = {
    .frequency = 10000,
    .channel = TIM2
};

static const struct UsbDeviceConfig usbConfig = {
    .dm = PIN(PORT_A, 11),
    .dp = PIN(PORT_A, 12),
    .vid = 0x15A2,
    .pid = 0x0044,
    .channel = 0
};
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 8000000
};

static const struct MainPllConfig mainPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 1,
    .multiplier = 9
};

static const struct BusClockConfig ahbClockConfig = {
    .divisor = 1
};

static const struct BusClockConfig apbClockConfigFast = {
    .divisor = 1
};

static const struct BusClockConfig apbClockConfigSlow = {
    .divisor = 2
};

static const struct SystemClockConfig systemClockConfigExt = {
    .source = CLOCK_EXTERNAL
};

static const struct SystemClockConfig systemClockConfigPll = {
    .source = CLOCK_PLL
};

static const struct UsbClockConfig usbClockConfig = {
    .divisor = USB_CLK_DIV_1_5
};
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(Apb1Clock, &apbClockConfigFast);
  clockEnable(Apb2Clock, &apbClockConfigFast);
  clockEnable(SystemClock, &systemClockConfigExt);

  clockEnable(MainClock, &ahbClockConfig);
}
/*----------------------------------------------------------------------------*/
void boardSetupClockPll(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainPll, &mainPllConfig);
  while (!clockReady(MainPll));

  clockEnable(Apb1Clock, &apbClockConfigSlow);
  clockEnable(Apb2Clock, &apbClockConfigFast);
  clockEnable(SystemClock, &systemClockConfigPll);
  clockEnable(UsbClock, &usbClockConfig);

  clockEnable(MainClock, &ahbClockConfig);
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupAdcTimer(void)
{
  struct Timer * const timer = init(GpTimer, &adcTimerConfig);
  assert(timer);
  return(timer);
}
/*----------------------------------------------------------------------------*/
struct Interrupt *boardSetupButton(void)
{
  struct Interrupt * const interrupt = init(Exti, &buttonIntConfig);
  assert(interrupt);
  return(interrupt);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupCan(struct Timer *timer)
{
  /* Override default config */
  struct CanConfig config = canConfig;
  config.timer = timer;

  struct Interface * const interface = init(Can, &config);
  assert(interface);
  return(interface);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerial(void)
{
  struct Interface * const interface = init(Serial, &serialConfig);
  assert(interface);
  return(interface);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerialDma(void)
{
  struct Interface * const interface = init(SerialDma, &serialDmaConfig);
  assert(interface);
  return(interface);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpi(void)
{
  struct Interface * const interface = init(Spi, &spiConfig);
  assert(interface);
  return(interface);
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimer(void)
{
  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  return(timer);
}
/*----------------------------------------------------------------------------*/
struct Entity *boardSetupUsb(void)
{
  struct Entity * const usb = init(UsbDevice, &usbConfig);
  assert(usb);
  return(usb);
}
