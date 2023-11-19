/*
 * stm32f1xx_default/shared/board.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/stm32/adc.h>
#include <halm/platform/stm32/adc_dma.h>
#include <halm/platform/stm32/can.h>
#include <halm/platform/stm32/clocking.h>
#include <halm/platform/stm32/exti.h>
#include <halm/platform/stm32/gptimer.h>
#include <halm/platform/stm32/i2c.h>
#include <halm/platform/stm32/iwdg.h>
#include <halm/platform/stm32/serial.h>
#include <halm/platform/stm32/serial_dma.h>
#include <halm/platform/stm32/spi.h>
#include <halm/platform/stm32/usb_device.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpiSdio(void)
    __attribute__((alias("boardSetupSpi")));
/*----------------------------------------------------------------------------*/
const PinNumber adcPinArray[] = {
    PIN(PORT_C, 0),
    PIN(PORT_C, 5),
    PIN(PORT_A, 0),
    0
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

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 8000000
};
/*----------------------------------------------------------------------------*/
DECLARE_WQ_IRQ(WQ_LP, FLASH_ISR)
/*----------------------------------------------------------------------------*/
size_t boardGetAdcPinCount(void)
{
  return ARRAY_SIZE(adcPinArray) - 1;
}
/*----------------------------------------------------------------------------*/
void boardSetAdcTimerRate(struct Timer *timer,
    size_t count __attribute__((unused)), uint32_t rate)
{
  timerSetOverflow(timer, timerGetFrequency(timer) / rate);
}
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void)
{
  static const struct SystemClockConfig systemClockConfigExt = {
      .source = CLOCK_EXTERNAL
  };

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
  static const struct MainPllConfig mainPllConfig = {
      .divisor = 1,
      .multiplier = 9,
      .source = CLOCK_EXTERNAL
  };
  static const struct SystemClockConfig systemClockConfigPll = {
      .source = CLOCK_PLL
  };

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainPll, &mainPllConfig);
  while (!clockReady(MainPll));

  clockEnable(Apb1Clock, &apbClockConfigSlow);
  clockEnable(Apb2Clock, &apbClockConfigFast);
  clockEnable(SystemClock, &systemClockConfigPll);

  clockEnable(MainClock, &ahbClockConfig);
}
/*----------------------------------------------------------------------------*/
void boardSetupLowPriorityWQ(void)
{
  static const struct WorkQueueIrqConfig wqIrqConfig = {
      .size = 4,
      .irq = FLASH_IRQ,
      .priority = 0
  };

  struct WorkQueue * const wq = init(WorkQueueIrq, &wqIrqConfig);
  assert(wq != NULL);
  (void)wq;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupAdc(void)
{
  static const struct AdcConfig adcConfig = {
      .pins = adcPinArray,
      .event = ADC_INJ_TIM3_CC4,
      .sensitivity = INPUT_TOGGLE,
      .channel = 0
  };

  struct Interface * const interface = init(Adc, &adcConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupAdcDma(void)
{
  static const struct AdcDmaConfig adcDmaConfig = {
      .pins = adcPinArray,
      .event = ADC_TIM3_TRGO,
      .sensitivity = INPUT_RISING,
      .channel = 0,
      .dma = DMA1_STREAM1
  };

  struct Interface * const interface = init(AdcDma, &adcDmaConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupAdcTimer(void)
{
  /* ADC triggers TIM3_TRGO and TIM3_CC4 */
  static const struct GpTimerConfig adcTimerConfig = {
      .frequency = 10000,
      .channel = TIM3,
      .event = TIM_EVENT_CC4
  };

  struct Timer * const timer = init(GpTimer, &adcTimerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Interrupt *boardSetupButton(void)
{
  static const struct ExtiConfig buttonIntConfig = {
      .pin = BOARD_BUTTON,
      .event = INPUT_FALLING,
      .pull = PIN_PULLUP
  };

  struct Interrupt * const interrupt = init(Exti, &buttonIntConfig);
  assert(interrupt != NULL);
  return interrupt;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupCan(struct Timer *timer)
{
  const struct CanConfig canConfig = {
      .timer = timer,
      .rate = 1000000,
      .rxBuffers = 4,
      .txBuffers = 4,
      .rx = PIN(PORT_B, 8),
      .tx = PIN(PORT_B, 9),
      .channel = CAN1
  };

  struct Interface * const interface = init(Can, &canConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupI2C(void)
{
  static const struct I2CConfig i2cConfig = {
      .rate = 100000,
      .scl = PIN(PORT_B, 6),
      .sda = PIN(PORT_B, 7),
      .channel = I2C1,
      .rxDma = DMA1_STREAM7,
      .txDma = DMA1_STREAM6
  };

  struct Interface * const interface = init(I2C, &i2cConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerial(void)
{
  static const struct SerialConfig serialConfig = {
      .rxLength = BOARD_UART_BUFFER,
      .txLength = BOARD_UART_BUFFER,
      .rate = 19200,
      .rx = PIN(PORT_B, 7),
      .tx = PIN(PORT_B, 6),
      .channel = USART1
  };

  struct Interface * const interface = init(Serial, &serialConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerialDma(void)
{
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

  struct Interface * const interface = init(SerialDma, &serialDmaConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpi(void)
{
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

  struct Interface * const interface = init(Spi, &spiConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimer(void)
{
  static const struct GpTimerConfig timerConfig = {
      .frequency = 10000,
      .channel = TIM2
  };

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Entity *boardSetupUsb(void)
{
  /* Clocks */
  static const struct UsbClockConfig usbClockConfig = {
      .divisor = USB_CLK_DIV_1_5
  };

  /* Objects */
  static const struct UsbDeviceConfig usbConfig = {
      .dm = PIN(PORT_A, 11),
      .dp = PIN(PORT_A, 12),
      .vid = 0x15A2,
      .pid = 0x0044,
      .channel = 0
  };

  clockEnable(UsbClock, &usbClockConfig);

  struct Entity * const usb = init(UsbDevice, &usbConfig);
  assert(usb != NULL);
  return usb;
}
/*----------------------------------------------------------------------------*/
struct Watchdog *boardSetupWdt(bool disarmed __attribute__((unused)))
{
  static const struct IwdgConfig iwdgConfig = {
      .period = 1000
  };

  clockEnable(InternalLowSpeedOsc, NULL);
  while (!clockReady(InternalLowSpeedOsc));

  struct Watchdog * const timer = init(Iwdg, &iwdgConfig);
  assert(timer != NULL);
  return timer;
}
