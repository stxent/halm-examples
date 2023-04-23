/*
 * m48x_default/shared/board.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/numicro/bpwm.h>
#include <halm/platform/numicro/can.h>
#include <halm/platform/numicro/clocking.h>
#include <halm/platform/numicro/eadc.h>
#include <halm/platform/numicro/eadc_dma.h>
#include <halm/platform/numicro/gptimer.h>
#include <halm/platform/numicro/hsusb_device.h>
#include <halm/platform/numicro/i2c.h>
#include <halm/platform/numicro/pin_int.h>
#include <halm/platform/numicro/serial.h>
#include <halm/platform/numicro/serial_dma.h>
#include <halm/platform/numicro/spi.h>
#include <halm/platform/numicro/spi_dma.h>
#include <halm/platform/numicro/qspi.h>
#include <halm/platform/numicro/usb_device.h>
#include <halm/platform/numicro/wdt.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
const PinNumber adcPinArray[] = {
    PIN(PORT_B, 0),
    PIN(PORT_B, 1),
    PIN(PORT_B, 2),
    PIN(PORT_B, 3),
    PIN(PORT_B, 12),
    PIN(PORT_B, 13),
    PIN(PORT_B, 14),
    PIN(PORT_B, 15),
    0
};

static const struct EadcConfig adcConfig = {
    .pins = adcPinArray,
    .event = ADC_TIMER1,
    .channel = 0
};

static const struct EadcDmaConfig adcDmaConfig = {
    .pins = adcPinArray,
    .event = ADC_TIMER1,
    .channel = 0,
    .dma = DMA0_CHANNEL0
};

static const struct GpTimerConfig adcTimerConfig = {
    .frequency = 1000000,
    .channel = 1,
    .trigger = {
        .adc = true
    }
};

static const struct BpwmUnitConfig bpwmTimerConfig = {
    .frequency = 1000000,
    .resolution = 20000,
    .channel = 0
};

static const struct PinIntConfig buttonIntConfig = {
    .pin = BOARD_BUTTON,
    .event = PIN_FALLING,
    .pull = PIN_NOPULL
};

static const struct CanConfig canConfig = {
    .rate = 1000000,
    .rxBuffers = 4,
    .txBuffers = 4,
    .rx = PIN(PORT_A, 4),
    .tx = PIN(PORT_A, 5),
    .channel = 0
};

static const struct UsbDeviceConfig hsUsbConfig = {
    .dm = PIN(PORT_HSUSB, PIN_HSUSB_DM),
    .dp = PIN(PORT_HSUSB, PIN_HSUSB_DP),
    .vbus = PIN(PORT_HSUSB, PIN_HSUSB_VBUS),
    .vid = 0x15A2,
    .pid = 0x0044,
    .channel = 0
};

static const struct I2CConfig i2cConfig = {
    .rate = 100000,
    .scl = PIN(PORT_G, 0),
    .sda = PIN(PORT_G, 1),
    .channel = 0
};

static const struct QspiConfig qspiConfig = {
    .rate = 1000000,
    .cs = 0,
    .io0 = PIN(PORT_C, 0),
    .io1 = PIN(PORT_C, 1),
    .io2 = PIN(PORT_C, 5),
    .io3 = PIN(PORT_C, 4),
    .sck = PIN(PORT_C, 2),
    .channel = 0,
    .mode = 0,
    .dma = {DMA0_CHANNEL0, DMA0_CHANNEL1}
};
};

static const struct SerialConfig serialConfig = {
    .rxLength = BOARD_UART_BUFFER,
    .txLength = BOARD_UART_BUFFER,
    .rate = 19200,
    .rx = PIN(PORT_A, 0),
    .tx = PIN(PORT_A, 1),
    .channel = 0
};

static const struct SerialDmaConfig serialDmaConfig = {
    .rxChunks = 8,
    .rxLength = BOARD_UART_BUFFER,
    .txLength = BOARD_UART_BUFFER,
    .rate = 19200,
    .rx = PIN(PORT_A, 0),
    .tx = PIN(PORT_A, 1),
    .channel = 0,
    .dma = {DMA0_CHANNEL0, DMA0_CHANNEL1}
};

static const struct SpiConfig spiConfig = {
    .rate = 2000000,
    .miso = PIN(PORT_C, 12),
    .mosi = PIN(PORT_C, 11),
    .sck = PIN(PORT_A, 2),
    .channel = 3,
    .mode = 0
};

static const struct SpiDmaConfig spiDmaConfig = {
    .rate = 2000000,
    .miso = PIN(PORT_C, 12),
    .mosi = PIN(PORT_C, 11),
    .sck = PIN(PORT_C, 10),
    .channel = 3,
    .mode = 0,
    .dma = {DMA0_CHANNEL0, DMA0_CHANNEL1}
};

static const struct GpTimerConfig timerConfig = {
    .frequency = 1000000,
    .channel = 0
};

static const struct UsbDeviceConfig usbConfig = {
    .dm = PIN(PORT_A, 13),
    .dp = PIN(PORT_A, 14),
    .vbus = PIN(PORT_A, 12),
    .vid = 0x15A2,
    .pid = 0x0044,
    .channel = 0
};

static const struct WdtConfig wdtConfig = {
    .period = 5000
};
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct PllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 1,
    .multiplier = 40
};

static const struct DividedClockConfig adcClockConfig = {
    .divisor = 2
};

static const struct ApbClockConfig apbClockConfigDirect = {
    .divisor = 1
};

static const struct ApbClockConfig apbClockConfigDivided = {
    .divisor = 2
};

static const struct ExtendedClockConfig bpwmClockConfig = {
    .source = CLOCK_APB,
    .divisor = 1
};

static const struct ExtendedClockConfig mainClockConfigExt = {
    .source = CLOCK_EXTERNAL,
    .divisor = 1
};

static const struct ExtendedClockConfig mainClockConfigPll = {
    .source = CLOCK_PLL,
    .divisor = 3
};

static const struct ExtendedClockConfig qspiClockConfig = {
    .source = CLOCK_APB,
    .divisor = 1
};

static const struct ExtendedClockConfig spiClockConfig = {
    .source = CLOCK_APB,
    .divisor = 1
};

static const struct ExtendedClockConfig uartClockConfig = {
    .source = CLOCK_APB,
    .divisor = 1
};

static const struct ExtendedClockConfig usbClockConfig = {
    .source = CLOCK_PLL,
    .divisor = 10
};

static const struct GenericClockConfig wdtClockConfig = {
    .source = CLOCK_INTERNAL_LS
};
/*----------------------------------------------------------------------------*/
size_t boardGetAdcPinCount(void)
{
  return ARRAY_SIZE(adcPinArray) - 1;
}
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(Apb0Clock, &apbClockConfigDirect);
  clockEnable(Apb1Clock, &apbClockConfigDirect);
  clockEnable(MainClock, &mainClockConfigExt);
}
/*----------------------------------------------------------------------------*/
void boardSetupClockPll(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(Apb0Clock, &apbClockConfigDivided);
  clockEnable(Apb1Clock, &apbClockConfigDivided);
  clockEnable(MainClock, &mainClockConfigPll);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupAdc(void)
{
  clockEnable(adcConfig.channel ? Eadc1Clock : Eadc0Clock, &adcClockConfig);

  struct Interface * const interface = init(Eadc, &adcConfig);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupAdcDma(void)
{
  clockEnable(adcDmaConfig.channel ? Eadc1Clock : Eadc0Clock, &adcClockConfig);

  struct Interface * const interface = init(EadcDma, &adcDmaConfig);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupAdcTimer(void)
{
  struct Timer * const timer = init(GpTimer, &adcTimerConfig);
  assert(timer);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct PwmPackage boardSetupBpwm(bool centered)
{
  const bool inversion = centered;

  clockEnable(bpwmTimerConfig.channel ? Bpwm1Clock : Bpwm0Clock,
      &bpwmClockConfig);

  /* Override default config */
  struct BpwmUnitConfig config = bpwmTimerConfig;
  config.centered = centered;

  struct BpwmUnit * const timer = init(BpwmUnit, &config);
  assert(timer);

  struct Pwm * const pwm0 = bpwmCreate(timer, BOARD_PWM_0, inversion);
  assert(pwm0);
  struct Pwm * const pwm1 = bpwmCreate(timer, BOARD_PWM_1, inversion);
  assert(pwm1);
  struct Pwm * const pwm2 = bpwmCreate(timer, BOARD_PWM_2, inversion);
  assert(pwm2);

  return (struct PwmPackage){
      (struct Timer *)timer,
      pwm0,
      {pwm0, pwm1, pwm2}
  };
}
/*----------------------------------------------------------------------------*/
struct Interrupt *boardSetupButton(void)
{
  struct Interrupt * const interrupt = init(PinInt, &buttonIntConfig);
  assert(interrupt);
  return interrupt;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupCan(struct Timer *timer)
{
  /* Override default config */
  struct CanConfig config = canConfig;
  config.timer = timer;

  struct Interface * const interface = init(Can, &config);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Entity *boardSetupHsUsb(void)
{
  assert(clockReady(ExternalOsc));

  struct Entity * const usb = init(HsUsbDevice, &hsUsbConfig);
  assert(usb);
  return usb;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupI2C(void)
{
  struct Interface * const interface = init(I2C, &i2cConfig);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupQspi(void)
{
  clockEnable(qspiConfig.channel ? Qspi1Clock : Qspi0Clock, &qspiClockConfig);

  struct Interface * const interface = init(Qspi, &qspiConfig);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerial(void)
{
  const void * const UART_CLOCKS[] = {
      Uart0Clock, Uart1Clock, Uart2Clock, Uart3Clock,
      Uart4Clock, Uart5Clock, Uart6Clock, Uart7Clock
  };

  clockEnable(UART_CLOCKS[serialConfig.channel], &uartClockConfig);

  struct Interface * const interface = init(Serial, &serialConfig);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerialDma(void)
{
  const void * const UART_CLOCKS[] = {
      Uart0Clock, Uart1Clock, Uart2Clock, Uart3Clock,
      Uart4Clock, Uart5Clock, Uart6Clock, Uart7Clock
  };

  clockEnable(UART_CLOCKS[serialConfig.channel], &uartClockConfig);

  struct Interface * const interface = init(SerialDma, &serialDmaConfig);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpi(void)
{
  clockEnable(Spi0Clock, &spiClockConfig);

  struct Interface * const interface = init(Spi, &spiConfig);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpiDma(void)
{
  clockEnable(Spi0Clock, &spiClockConfig);

  struct Interface * const interface = init(SpiDma, &spiDmaConfig);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimer(void)
{
  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Entity *boardSetupUsb(void)
{
  assert(clockReady(SystemPll));
  clockEnable(UsbClock, &usbClockConfig);

  struct Entity * const usb = init(UsbDevice, &usbConfig);
  assert(usb);
  return usb;
}
/*----------------------------------------------------------------------------*/
struct Watchdog *boardSetupWdt(bool disarmed)
{
  clockEnable(WdtClock, &wdtClockConfig);

  /* Override default config */
  struct WdtConfig config = wdtConfig;
  config.disarmed = disarmed;

  struct Watchdog * const timer = init(Wdt, &config);
  assert(timer);
  return timer;
}
