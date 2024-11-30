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
#include <halm/platform/numicro/flash.h>
#include <halm/platform/numicro/gptimer.h>
#include <halm/platform/numicro/hsusb_device.h>
#include <halm/platform/numicro/i2c.h>
#include <halm/platform/numicro/pin_int.h>
#include <halm/platform/numicro/sdh.h>
#include <halm/platform/numicro/serial.h>
#include <halm/platform/numicro/serial_dma.h>
#include <halm/platform/numicro/spi.h>
#include <halm/platform/numicro/spi_dma.h>
#include <halm/platform/numicro/spim.h>
#include <halm/platform/numicro/qspi.h>
#include <halm/platform/numicro/usb_device.h>
#include <halm/platform/numicro/wdt.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
[[gnu::alias("boardSetupPwmBPWM")]] struct PwmPackage boardSetupPwm(bool);

[[gnu::alias("boardSetupUsbHs")]] struct Usb *boardSetupUsb(void);
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

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct DividedClockConfig adcClockConfig = {
    .divisor = 2
};

static const struct ExtendedClockConfig bpwmClockConfig = {
    .divisor = 1,
    .source = CLOCK_APB
};

static const struct ExtendedClockConfig qspiClockConfig = {
    .divisor = 1,
    .source = CLOCK_APB
};

static const struct GenericClockConfig sdhClockConfig = {
    .source = CLOCK_EXTERNAL
};

static const struct ExtendedClockConfig spiClockConfig = {
    .divisor = 1,
    .source = CLOCK_APB
};

static const struct ExtendedClockConfig uartClockConfig = {
    .divisor = 1,
    .source = CLOCK_PLL
};
/*----------------------------------------------------------------------------*/
size_t boardGetAdcPinCount(void)
{
  return ARRAY_SIZE(adcPinArray) - 1;
}
/*----------------------------------------------------------------------------*/
void boardSetAdcTimerRate(struct Timer *timer, size_t, uint32_t rate)
{
  timerSetOverflow(timer, timerGetFrequency(timer) / rate);
}
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void)
{
  static const struct ApbClockConfig apbClockConfigDirect = {
      .divisor = 1
  };
  static const struct ExtendedClockConfig mainClockConfigExt = {
      .divisor = 1,
      .source = CLOCK_EXTERNAL
  };

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(Apb0Clock, &apbClockConfigDirect);
  clockEnable(Apb1Clock, &apbClockConfigDirect);
  clockEnable(MainClock, &mainClockConfigExt);
}
/*----------------------------------------------------------------------------*/
void boardSetupClockPll(void)
{
  static const struct ApbClockConfig apbClockConfigDivided = {
      .divisor = 2
  };
  static const struct ExtendedClockConfig mainClockConfigPll = {
      .divisor = 3,
      .source = CLOCK_PLL
  };
  static const struct PllConfig systemPllConfig = {
      .divisor = 1,
      .multiplier = 40,
      .source = CLOCK_EXTERNAL
  };

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &systemPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(Apb0Clock, &apbClockConfigDivided);
  clockEnable(Apb1Clock, &apbClockConfigDivided);
  clockEnable(MainClock, &mainClockConfigPll);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupAdc(void)
{
  static const struct EadcConfig adcConfig = {
      .pins = adcPinArray,
      .event = ADC_TIMER1,
      .channel = 0
  };

  clockEnable(adcConfig.channel ? Eadc1Clock : Eadc0Clock, &adcClockConfig);

  struct Interface * const interface = init(Eadc, &adcConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupAdcDma(void)
{
  static const struct EadcDmaConfig adcDmaConfig = {
      .pins = adcPinArray,
      .event = ADC_TIMER1,
      .channel = 0,
      .dma = DMA0_CHANNEL0
  };

  clockEnable(adcDmaConfig.channel ? Eadc1Clock : Eadc0Clock, &adcClockConfig);

  struct Interface * const interface = init(EadcDma, &adcDmaConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupAdcTimer(void)
{
  static const struct GpTimerConfig adcTimerConfig = {
      .frequency = 1000000,
      .channel = 1,
      .trigger = {
          .adc = true
      }
  };

  struct Timer * const timer = init(GpTimer, &adcTimerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Interrupt *boardSetupButton(void)
{
  static const struct PinIntConfig buttonIntConfig = {
      .pin = BOARD_BUTTON,
      .event = BOARD_BUTTON_INV ? INPUT_FALLING : INPUT_RISING,
      .pull = PIN_NOPULL
  };

  struct Interrupt * const interrupt = init(PinInt, &buttonIntConfig);
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
      .rx = PIN(PORT_A, 4),
      .tx = PIN(PORT_A, 5),
      .channel = 0
  };

  struct Interface * const interface = init(Can, &canConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupFlash(void)
{
  static const struct FlashConfig flashConfig = {
      .bank = FLASH_APROM
  };

  struct Interface * const interface = init(Flash, &flashConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupI2C(void)
{
  static const struct I2CConfig i2cConfig = {
      .rate = 100000,
      .scl = PIN(PORT_G, 0),
      .sda = PIN(PORT_G, 1),
      .channel = 0
  };

  struct Interface * const interface = init(I2C, &i2cConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct PwmPackage boardSetupPwmBPWM(bool centered)
{
  const struct BpwmUnitConfig bpwmTimerConfig = {
      .frequency = 1000000,
      .resolution = 20000,
      .channel = 0,
      .centered = centered
  };
  const bool inversion = centered;

  clockEnable(bpwmTimerConfig.channel ? Bpwm1Clock : Bpwm0Clock,
      &bpwmClockConfig);

  struct BpwmUnit * const timer = init(BpwmUnit, &bpwmTimerConfig);
  assert(timer != NULL);

  struct Pwm * const pwm0 = bpwmCreate(timer, BOARD_BPWM_0, inversion);
  assert(pwm0 != NULL);
  struct Pwm * const pwm1 = bpwmCreate(timer, BOARD_BPWM_1, inversion);
  assert(pwm1 != NULL);

  return (struct PwmPackage){
      (struct Timer *)timer,
      pwm0,
      {pwm0, pwm1, NULL}
  };
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupQspi(void)
{
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

  clockEnable(qspiConfig.channel ? Qspi1Clock : Qspi0Clock, &qspiClockConfig);

  struct Interface * const interface = init(Qspi, &qspiConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSdio(bool wide, struct Timer *timer)
{
  const struct SdhConfig sdhConfig1Bit = {
      .timer = timer,
      .rate = 1000000,
      .clk = PIN(PORT_E, 6),
      .cmd = PIN(PORT_E, 7),
      .dat0 = PIN(PORT_E, 2),
      .channel = 0
  };
  const struct SdhConfig sdhConfig4Bit = {
      .timer = timer,
      .rate = 1000000,
      .clk = PIN(PORT_E, 6),
      .cmd = PIN(PORT_E, 7),
      .dat0 = PIN(PORT_E, 2),
      .dat1 = PIN(PORT_E, 3),
      .dat2 = PIN(PORT_B, 4),
      .dat3 = PIN(PORT_B, 5),
      .channel = 0
  };

  const struct SdhConfig * const config = wide ?
      &sdhConfig4Bit : &sdhConfig1Bit;

  clockEnable(config->channel ? Sdh1Clock : Sdh0Clock, &sdhClockConfig);

  struct Interface * const interface = init(Sdh, config);
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
      .rx = PIN(PORT_A, 0),
      .tx = PIN(PORT_A, 1),
      .channel = 0
  };
  const void * const UART_CLOCKS[] = {
      Uart0Clock, Uart1Clock, Uart2Clock, Uart3Clock,
      Uart4Clock, Uart5Clock, Uart6Clock, Uart7Clock
  };

  clockEnable(UART_CLOCKS[serialConfig.channel], &uartClockConfig);

  struct Interface * const interface = init(Serial, &serialConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpi(void)
{
  static const struct SpiConfig spiConfig = {
      .rate = 2000000,
      .miso = PIN(PORT_C, 12),
      .mosi = PIN(PORT_C, 11),
      .sck = PIN(PORT_A, 2),
      .channel = 3,
      .mode = 0
  };

  clockEnable(Spi0Clock, &spiClockConfig);

  struct Interface * const interface = init(Spi, &spiConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpiDma(void)
{
  static const struct SpiDmaConfig spiDmaConfig = {
      .rate = 2000000,
      .miso = PIN(PORT_C, 12),
      .mosi = PIN(PORT_C, 11),
      .sck = PIN(PORT_C, 10),
      .channel = 3,
      .mode = 0,
      .dma = {DMA0_CHANNEL0, DMA0_CHANNEL1}
  };

  clockEnable(Spi0Clock, &spiClockConfig);

  struct Interface * const interface = init(SpiDma, &spiDmaConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpim(struct Timer *timer)
{
  const struct SpimConfig spimConfig = {
      .timer = timer,
      .rate = 2000000,
      .cs = PIN(PORT_C, 3),
      .io0 = PIN(PORT_C, 0),
      .io1 = PIN(PORT_C, 1),
      .io2 = PIN(PORT_C, 5),
      .io3 = PIN(PORT_C, 4),
      .sck = PIN(PORT_C, 2),
      .channel = 0
  };

  struct Interface * const interface = init(Spim, &spimConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimer(void)
{
  static const struct GpTimerConfig timerConfig = {
      .frequency = 1000000,
      .channel = 0
  };

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Usb *boardSetupUsbFs(void)
{
  /* Clocks */
  static const struct ExtendedClockConfig usbClockConfig = {
      .divisor = 10,
      .source = CLOCK_PLL
  };

  /* Objects */
  static const struct UsbDeviceConfig usbConfig = {
      .dm = PIN(PORT_A, 13),
      .dp = PIN(PORT_A, 14),
      .vbus = PIN(PORT_A, 12),
      .vid = 0x15A2,
      .pid = 0x0044,
      .channel = 0
  };

  assert(clockReady(SystemPll));
  clockEnable(UsbClock, &usbClockConfig);

  struct Usb * const usb = init(UsbDevice, &usbConfig);
  assert(usb != NULL);
  return usb;
}
/*----------------------------------------------------------------------------*/
struct Usb *boardSetupUsbHs(void)
{
  static const struct UsbDeviceConfig hsUsbConfig = {
      .dm = PIN(PORT_HSUSB, PIN_HSUSB_DM),
      .dp = PIN(PORT_HSUSB, PIN_HSUSB_DP),
      .vbus = PIN(PORT_HSUSB, PIN_HSUSB_VBUS),
      .vid = 0x15A2,
      .pid = 0x0044,
      .channel = 0
  };

  assert(clockReady(ExternalOsc));

  struct Usb * const usb = init(HsUsbDevice, &hsUsbConfig);
  assert(usb != NULL);
  return usb;
}
/*----------------------------------------------------------------------------*/
struct Watchdog *boardSetupWdt(bool disarmed)
{
  /* Clocks */
  static const struct GenericClockConfig wdtClockConfig = {
      .source = CLOCK_INTERNAL_LS
  };

  /* Objects */
  static const struct WdtConfig wdtConfig = {
      .period = 5000
  };

  clockEnable(WdtClock, &wdtClockConfig);

  /* Override default config */
  struct WdtConfig config = wdtConfig;
  config.disarmed = disarmed;

  struct Watchdog * const timer = init(Wdt, &config);
  assert(timer != NULL);
  return timer;
}
