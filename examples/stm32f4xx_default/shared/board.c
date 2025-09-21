/*
 * stm32f4xx_default/shared/board.c
 * Copyright (C) 2024 xent
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
#include <halm/platform/stm32/sdio.h>
#include <halm/platform/stm32/serial.h>
#include <halm/platform/stm32/serial_dma.h>
#include <halm/platform/stm32/spi.h>
#include <halm/platform/stm32/usb_device.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
[[gnu::alias("boardSetupTimer2")]] struct Timer *boardSetupAdcTimer(void);
[[gnu::alias("boardSetupTimer5")]] struct Timer *boardSetupTimer(void);
[[gnu::alias("boardSetupTimer2")]] struct Timer *boardSetupTimerAux(void);

[[gnu::alias("boardSetupI2C1")]] struct Interface *boardSetupI2C(void);

[[gnu::alias("boardSetupSpi1")]] struct Interface *boardSetupSpi(void);
[[gnu::alias("boardSetupSpi2")]] struct Interface *boardSetupSpiSdio(void);
/*----------------------------------------------------------------------------*/
const PinNumber adcPinArray[] = {
    PIN(PORT_A, 0),
    PIN(PORT_C, 0),
    PIN(PORT_C, 5),
    0
};

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 8000000
};

static const struct MainClockConfig mainClockConfig = {
    .divisor = 1,
    .range = VR_2V7_3V6
};
/*----------------------------------------------------------------------------*/
DECLARE_WQ_IRQ(WQ_LP, FLASH_ISR)
/*----------------------------------------------------------------------------*/
size_t boardGetAdcPinCount(void)
{
  return ARRAY_SIZE(adcPinArray) - 1;
}
/*----------------------------------------------------------------------------*/
void boardSetAdcTimerRate(struct Timer *timer, size_t, unsigned int rate)
{
  timerSetOverflow(timer, timerGetFrequency(timer) / rate);
}
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void)
{
  static const struct BusClockConfig apbClockConfigBypass = {
      .divisor = 1
  };
  static const struct SystemClockConfig systemClockConfigExt = {
      .source = CLOCK_EXTERNAL
  };

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(Apb1Clock, &apbClockConfigBypass);
  clockEnable(Apb2Clock, &apbClockConfigBypass);
  clockEnable(SystemClock, &systemClockConfigExt);

  clockEnable(MainClock, &mainClockConfig);
}
/*----------------------------------------------------------------------------*/
void boardSetupClockPll(void)
{
  static const struct BusClockConfig apbClockConfigFast = {
      .divisor = 2
  };
  static const struct BusClockConfig apbClockConfigSlow = {
      .divisor = 4
  };
  static const struct MainPllConfig mainPllConfig = {
      .divisor = 2,
      .multiplier = 42,
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

  clockEnable(MainClock, &mainClockConfig);
}
/*----------------------------------------------------------------------------*/
void boardSetupLowPriorityWQ(void)
{
  static const struct WorkQueueIrqConfig wqIrqConfig = {
      .size = 4,
      .irq = FLASH_IRQ,
      .priority = 0
  };

  WQ_LP = init(WorkQueueIrq, &wqIrqConfig);
  assert(WQ_LP != NULL);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupAdc(void)
{
  static const struct AdcConfig adcConfig = {
      .pins = adcPinArray,
      .event = ADC_INJ_TIM2_TRGO,
      .sensitivity = INPUT_RISING,
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
      .event = ADC_TIM2_CC2,
      .sensitivity = INPUT_TOGGLE,
      .channel = 0,
      .dma = DMA2_STREAM0
  };

  struct Interface * const interface = init(AdcDma, &adcDmaConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interrupt *boardSetupButton(void)
{
  static const struct ExtiConfig buttonIntConfig = {
      .pin = BOARD_BUTTON,
      .event = BOARD_BUTTON_INV ? INPUT_FALLING : INPUT_RISING,
      .pull = BOARD_BUTTON_INV ? PIN_PULLUP : PIN_PULLDOWN
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
      .rate = 100000,
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
struct Interface *boardSetupI2C1(void)
{
  static const struct I2CConfig i2cConfig = {
      .rate = 100000,
      .scl = PIN(PORT_B, 6),
      .sda = PIN(PORT_B, 7),
      .channel = I2C1,
      .rxDma = DMA1_STREAM0,
      .txDma = DMA1_STREAM6
  };

  struct Interface * const interface = init(I2C, &i2cConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupI2C2(void)
{
  static const struct I2CConfig i2cConfig = {
      .rate = 100000,
      .scl = PIN(PORT_B, 10),
      .sda = PIN(PORT_B, 11),
      .channel = I2C2,
      .rxDma = DMA1_STREAM2,
      .txDma = DMA1_STREAM7
  };

  struct Interface * const interface = init(I2C, &i2cConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSdio(bool wide, struct Timer *timer)
{
  const struct SdioConfig sdioConfig1Bit = {
      .timer = timer,
      .rate = 1000000,
      .clk = PIN(PORT_C, 12),
      .cmd = PIN(PORT_D, 2),
      .dat0 = PIN(PORT_C, 8),
      .dma = {
          .priority = DMA_PRIORITY_LOW,
          .stream = DMA2_STREAM3
      }
  };
  const struct SdioConfig sdioConfig4Bit = {
      .timer = timer,
      .rate = 1000000,
      .clk = PIN(PORT_C, 12),
      .cmd = PIN(PORT_D, 2),
      .dat0 = PIN(PORT_C, 8),
      .dat1 = PIN(PORT_C, 9),
      .dat2 = PIN(PORT_C, 10),
      .dat3 = PIN(PORT_C, 11),
      .dma = {
          .priority = DMA_PRIORITY_HIGH,
          .stream = DMA2_STREAM3
      }
  };

  struct Interface * const interface = init(Sdio,
      wide ? &sdioConfig4Bit : &sdioConfig1Bit);
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
      .rxDma = DMA2_STREAM2,
      .txDma = DMA2_STREAM7
  };

  struct Interface * const interface = init(SerialDma, &serialDmaConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpi1(void)
{
  static const struct SpiConfig spiConfig = {
      .rate = 2000000,
      .miso = PIN(PORT_A, 6),
      .mosi = PIN(PORT_A, 7),
      .sck = PIN(PORT_A, 5),
      .channel = SPI1,
      .mode = 0,
      .rxDma = DMA2_STREAM2,
      .txDma = DMA2_STREAM3
  };

  struct Interface * const interface = init(Spi, &spiConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpi2(void)
{
  static const struct SpiConfig spiConfig = {
      .rate = 2000000,
      .miso = PIN(PORT_B, 14),
      .mosi = PIN(PORT_B, 15),
      .sck = PIN(PORT_B, 13),
      .channel = SPI2,
      .mode = 0,
      .rxDma = DMA1_STREAM3,
      .txDma = DMA1_STREAM4
  };

  struct Interface * const interface = init(Spi, &spiConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimer2(void)
{
  /* ADC triggers TIM2_TRGO and TIM2_CC2 */
  static const struct GpTimerConfig timerConfig = {
      .frequency = 10000,
      .channel = TIM2,
      .event = TIM_EVENT_CC2
  };

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimer5(void)
{
  static const struct GpTimerConfig timerConfig = {
      .frequency = 10000,
      .channel = TIM5
  };

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Usb *boardSetupUsb(void)
{
  static const struct UsbDeviceConfig usbConfig = {
      .dm = PIN(PORT_A, 11),
      .dp = PIN(PORT_A, 12),
      .vid = 0x15A2,
      .pid = 0x0044,
      .channel = 0
  };

  struct Usb * const usb = init(UsbDevice, &usbConfig);
  assert(usb != NULL);
  return usb;
}
/*----------------------------------------------------------------------------*/
struct Watchdog *boardSetupWdt(bool)
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
