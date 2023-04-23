/*
 * lpc43xx_default/shared/board.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/lpc/adc.h>
#include <halm/platform/lpc/adc_dma.h>
#include <halm/platform/lpc/adc_dma_stream.h>
#include <halm/platform/lpc/adc_oneshot.h>
#include <halm/platform/lpc/can.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/dac.h>
#include <halm/platform/lpc/dac_dma.h>
#include <halm/platform/lpc/gptimer.h>
#include <halm/platform/lpc/i2c.h>
#include <halm/platform/lpc/i2c_slave.h>
#include <halm/platform/lpc/i2s_dma.h>
#include <halm/platform/lpc/pin_int.h>
#include <halm/platform/lpc/rit.h>
#include <halm/platform/lpc/rtc.h>
#include <halm/platform/lpc/sdmmc.h>
#include <halm/platform/lpc/serial.h>
#include <halm/platform/lpc/serial_dma.h>
#include <halm/platform/lpc/spi.h>
#include <halm/platform/lpc/spi_dma.h>
#include <halm/platform/lpc/spifi.h>
#include <halm/platform/lpc/system.h>
#include <halm/platform/lpc/sct_pwm.h>
#include <halm/platform/lpc/usb_device.h>
#include <halm/platform/lpc/wdt.h>
#include <halm/platform/lpc/wwdt.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupI2C(void)
    __attribute__((alias("boardSetupI2C1")));
struct Interface *boardSetupI2CSlave(void)
    __attribute__((alias("boardSetupI2CSlave1")));

struct Interface *boardSetupSpi(void)
    __attribute__((alias("boardSetupSpi0")));
struct Interface *boardSetupSpiDma(void)
    __attribute__((alias("boardSetupSpiDma0")));

static void enablePeriphClock(const void *);
/*----------------------------------------------------------------------------*/
const PinNumber adcPinArray[] = {
    PIN(PORT_ADC, 1),
    PIN(PORT_ADC, 2),
    PIN(PORT_ADC, 3),
    PIN(PORT_ADC, 5),
    0
};

static const struct AdcConfig adcConfig = {
    .pins = adcPinArray,
    .event = ADC_CTOUT_15,
    .channel = 0
};

static const struct AdcDmaConfig adcDmaConfig = {
    .pins = adcPinArray,
    .event = ADC_CTOUT_15,
    .channel = 0,
    .dma = 4
};

static const struct AdcDmaStreamConfig adcStreamConfig = {
    .pins = adcPinArray,
    .size = 2,
    .converter = {ADC_CTOUT_15, 0},
    .memory = {GPDMA_MAT0_0, 1},
    .channel = 0
};

static const struct AdcOneShotConfig adcOneShotConfig = {
    .pin = PIN(PORT_ADC, 7),
    .channel = 0
};

static const struct GpTimerConfig adcTimerConfig = {
    .frequency = 4000000,
    .event = GPTIMER_MATCH3,
    .channel = 3
};

static const struct PinIntConfig buttonIntConfig = {
    .pin = BOARD_BUTTON,
    .event = PIN_FALLING,
    .pull = PIN_PULLUP
};

static const struct CanConfig canConfig = {
    .timer = 0,
    .rate = 1000000,
    .rxBuffers = 4,
    .txBuffers = 4,
    .rx = PIN(PORT_3, 1),
    .tx = PIN(PORT_3, 2),
    .channel = 0
};

static const struct DacConfig dacConfig = {
    .pin = PIN(PORT_4, 4),
    .value = 32768
};

static const struct DacDmaConfig dacDmaConfig = {
    .size = 2,
    .rate = 96000,
    .value = 32768,
    .pin = PIN(PORT_4, 4),
    .dma = 5
};

static const struct I2CConfig i2c0Config = {
    .rate = 100000,
    .scl = PIN(PORT_I2C, PIN_I2C0_SCL),
    .sda = PIN(PORT_I2C, PIN_I2C0_SDA),
    .channel = 0
};

static const struct I2CConfig i2c1Config = {
    .rate = 100000,
    .scl = PIN(PORT_2, 4),
    .sda = PIN(PORT_2, 3),
    .channel = 1
};

static const struct I2CSlaveConfig i2cSlave0Config = {
    .size = 16,
    .scl = PIN(PORT_I2C, PIN_I2C0_SCL),
    .sda = PIN(PORT_I2C, PIN_I2C0_SDA),
    .channel = 0
};

static const struct I2CSlaveConfig i2cSlave1Config = {
    .size = 16,
    .scl = PIN(PORT_2, 4),
    .sda = PIN(PORT_2, 3),
    .channel = 1
};

static const struct I2SDmaConfig i2sConfig = {
    .size = 2,
    .rate = 44100,
    .width = I2S_WIDTH_16,
    .tx = {
        .sda = PIN(PORT_7, 2),
        .sck = PIN(PORT_4, 7),
        .ws = PIN(PORT_7, 1),
        .mclk = PIN(PORT_CLK, 2),
        .dma = 6
    },
    .rx = {
        .sda = PIN(PORT_6, 2),
        .dma = 7
    },
    .channel = 0,
    .mono = false,
    .slave = false
};

static const struct SctPwmUnitConfig pwmTimerUnifiedConfig = {
    .frequency = 1000000,
    .resolution = 20000,
    .part = SCT_UNIFIED,
    .channel = 0
};

static const struct SctPwmUnitConfig pwmTimerConfig = {
    .frequency = 1000000,
    .resolution = 20000,
    .part = SCT_HIGH,
    .channel = 0
};

static const struct RtcConfig rtcConfig = {
    /* January 1, 2017, 00:00:00 */
    .timestamp = 1483228800
};

static const struct SdmmcConfig sdmmcConfig1Bit = {
    .rate = 1000000,
    .clk = PIN(PORT_CLK, 0),
    .cmd = PIN(PORT_1, 6),
    .dat0 = PIN(PORT_1, 9)
};

static const struct SdmmcConfig sdmmcConfig4Bit = {
    .rate = 1000000,
    .clk = PIN(PORT_CLK, 0),
    .cmd = PIN(PORT_1, 6),
    .dat0 = PIN(PORT_1, 9),
    .dat1 = PIN(PORT_1, 10),
    .dat2 = PIN(PORT_1, 11),
    .dat3 = PIN(PORT_1, 12)
};

static const struct SerialConfig serialConfig = {
    .rxLength = BOARD_UART_BUFFER,
    .txLength = BOARD_UART_BUFFER,
    .rate = 19200,
    .rx = PIN(PORT_1, 14),
    .tx = PIN(PORT_5, 6),
    .channel = 1
};

static const struct SerialDmaConfig serialDmaConfig = {
    .rxChunks = 4,
    .rxLength = BOARD_UART_BUFFER,
    .txLength = BOARD_UART_BUFFER,
    .rate = 19200,
    .rx = PIN(PORT_1, 14),
    .tx = PIN(PORT_5, 6),
    .channel = 1,
    .dma = {2, 3}
};

static const struct SpiConfig spi0Config = {
    .rate = 2000000,
    .sck = PIN(PORT_3, 0),
    .miso = PIN(PORT_1, 1),
    .mosi = PIN(PORT_1, 2),
    .channel = 0,
    .mode = 0
};

static const struct SpiConfig spi1Config = {
    .rate = 2000000,
    .sck = PIN(PORT_F, 4),
    .miso = PIN(PORT_1, 3),
    .mosi = PIN(PORT_1, 4),
    .channel = 1,
    .mode = 0
};

static const struct SpiDmaConfig spiDma0Config = {
    .rate = 2000000,
    .sck = PIN(PORT_3, 0),
    .miso = PIN(PORT_1, 1),
    .mosi = PIN(PORT_1, 2),
    .channel = 0,
    .mode = 0,
    .dma = {0, 1}
};

static const struct SpiDmaConfig spiDma1Config = {
    .rate = 2000000,
    .sck = PIN(PORT_F, 4),
    .miso = PIN(PORT_1, 3),
    .mosi = PIN(PORT_1, 4),
    .channel = 1,
    .mode = 0,
    .dma = {0, 1}
};

static const struct SpifiConfig spifiConfig = {
    .cs = PIN(PORT_3, 8),
    .io0 = PIN(PORT_3, 7),
    .io1 = PIN(PORT_3, 6),
    .io2 = PIN(PORT_3, 5),
    .io3 = PIN(PORT_3, 4),
    .sck = PIN(PORT_3, 3),
    .channel = 0,
    .mode = 0,
    .dma = 0
};

static const struct GpTimerConfig timerConfig = {
    .frequency = 1000000,
    .event = GPTIMER_MATCH0,
    .channel = 0
};

static const struct UsbDeviceConfig usb0Config = {
    .dm = PIN(PORT_USB, PIN_USB0_DM),
    .dp = PIN(PORT_USB, PIN_USB0_DP),
    .connect = 0,
    .vbus = PIN(PORT_USB, PIN_USB0_VBUS),
    .vid = 0x15A2,
    .pid = 0x0044,
    .channel = 0
};

static const struct UsbDeviceConfig usb1Config = {
    .dm = PIN(PORT_USB, PIN_USB1_DM),
    .dp = PIN(PORT_USB, PIN_USB1_DP),
    .connect = 0,
    .vbus = PIN(PORT_2, 5),
    .vid = 0x15A2,
    .pid = 0x0044,
    .channel = 1
};

static const struct WdtConfig wdtConfig = {
    .period = 5000
};

static const struct WwdtConfig wwdtConfig = {
    .period = 5000,
    .window = 1000,
    .disarmed = false
};
/*----------------------------------------------------------------------------*/
static const struct PllConfig audioPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 4,
    .multiplier = 40
};

static const struct GenericDividerConfig divBConfig = {
    .source = CLOCK_AUDIO_PLL,
    .divisor = 3
};

static const struct GenericClockConfig divBClockSource = {
    .source = CLOCK_IDIVB
};

static const struct GenericDividerConfig divCConfig = {
    .source = CLOCK_AUDIO_PLL,
    .divisor = 2
};

static const struct GenericClockConfig divCClockSource = {
    .source = CLOCK_IDIVC
};

static const struct GenericDividerConfig divDConfig = {
    .source = CLOCK_PLL,
    .divisor = 2
};

static const struct GenericClockConfig divDClockSource = {
    .source = CLOCK_IDIVD
};

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct PllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 2,
    .multiplier = 17
};

static const struct PllConfig usbPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 1,
    .multiplier = 40
};

static const struct GenericClockConfig mainClockConfigExt = {
    .source = CLOCK_EXTERNAL
};

static const struct GenericClockConfig mainClockConfigInt = {
    .source = CLOCK_INTERNAL
};

static const struct GenericClockConfig mainClockConfigPll = {
    .source = CLOCK_PLL
};

static const struct GenericClockConfig usb0ClockConfig = {
    .source = CLOCK_USB_PLL
};
/*----------------------------------------------------------------------------*/
static void enablePeriphClock(const void *clock)
{
  if (!clockReady(clock))
  {
    if (clockReady(SystemPll))
      clockEnable(clock, &mainClockConfigPll);
    else if (clockReady(ExternalOsc))
      clockEnable(clock, &mainClockConfigExt);
    else
      clockEnable(clock, &mainClockConfigInt);

    while (!clockReady(clock));
  }
}
/*----------------------------------------------------------------------------*/
size_t boardGetAdcPinCount(void)
{
  return ARRAY_SIZE(adcPinArray) - 1;
}
/*----------------------------------------------------------------------------*/
void boardResetClock(void)
{
  clockEnable(MainClock, &mainClockConfigInt);

  if (clockReady(AudioPll))
    clockDisable(AudioPll);

  if (clockReady(UsbPll))
    clockDisable(UsbPll);

  if (clockReady(SystemPll))
    clockDisable(SystemPll);

  if (clockReady(ExternalOsc))
    clockDisable(ExternalOsc);

  /* Flash latency should be reset to exit correctly from power-down modes */
  sysFlashLatencyReset();
}
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void)
{
  clockEnable(MainClock, &mainClockConfigInt);

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainClock, &mainClockConfigExt);
}
/*----------------------------------------------------------------------------*/
void boardSetupClockInt(void)
{
  clockEnable(MainClock, &mainClockConfigInt);
}
/*----------------------------------------------------------------------------*/
void boardSetupClockPll(void)
{
  clockEnable(MainClock, &mainClockConfigInt);

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &mainClockConfigPll);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupAdc(void)
{
  /* ADC0 and ADC1 are connected to the APB3 bus */
  enablePeriphClock(Apb3Clock);

  struct Interface * const interface = init(Adc, &adcConfig);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupAdcDma(void)
{
  /* ADC0 and ADC1 are connected to the APB3 bus */
  enablePeriphClock(Apb3Clock);

  struct Interface * const interface = init(AdcDma, &adcDmaConfig);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupAdcOneShot(void)
{
  /* ADC0 and ADC1 are connected to the APB3 bus */
  enablePeriphClock(Apb3Clock);

  struct Interface * const interface = init(AdcOneShot, &adcOneShotConfig);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct StreamPackage boardSetupAdcStream(void)
{
  /* ADC0 and ADC1 are connected to the APB3 bus */
  enablePeriphClock(Apb3Clock);

  struct AdcDmaStream * const interface = init(AdcDmaStream, &adcStreamConfig);
  assert(interface);

  struct Stream * const stream = adcDmaStreamGetInput(interface);
  assert(stream);

  return (struct StreamPackage){(struct Interface *)interface, stream, 0};
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupAdcTimer(void)
{
  struct Timer * const timer = init(GpTimer, &adcTimerConfig);
  assert(timer);
  return timer;
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
  /* Make 120 MHz clock on AUDIO PLL */
  if (!clockReady(AudioPll))
  {
    clockEnable(AudioPll, &audioPllConfig);
    while (!clockReady(AudioPll));
  }

  /* Make 40 MHz clock for CAN, clock should be less than 50 MHz */
  clockEnable(DividerB, &divBConfig);
  while (!clockReady(DividerB));

  /* Override default config */
  struct CanConfig config = canConfig;
  config.timer = timer;

  if (config.channel == 0)
  {
    /* CAN0 is connected to the APB3 bus */
    clockEnable(Apb3Clock, &divBClockSource);
    while (!clockReady(Apb3Clock));
  }
  else
  {
    /* CAN1 is connected to the APB1 bus */
    clockEnable(Apb1Clock, &divBClockSource);
    while (!clockReady(Apb1Clock));
  }

  struct Interface * const interface = init(Can, &config);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupDac(void)
{
  /* DAC is connected to the APB3 bus */
  enablePeriphClock(Apb3Clock);

  struct Interface * const interface = init(Dac, &dacConfig);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct StreamPackage boardSetupDacDma(void)
{
  /* DAC is connected to the APB3 bus */
  enablePeriphClock(Apb3Clock);

  struct DacDma * const interface = init(DacDma, &dacDmaConfig);
  assert(interface);

  struct Stream * const stream = dacDmaGetOutput(interface);
  assert(stream);

  return (struct StreamPackage){(struct Interface *)interface, 0, stream};
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupI2C0(void)
{
  /* I2C0 is connected to the APB1 bus */
  enablePeriphClock(Apb1Clock);

  struct Interface * const interface = init(I2C, &i2c0Config);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupI2C1(void)
{
  /* I2C1 is connected to the APB3 bus */
  enablePeriphClock(Apb3Clock);

  struct Interface * const interface = init(I2C, &i2c1Config);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupI2CSlave0(void)
{
  /* I2C0 is connected to the APB1 bus */
  enablePeriphClock(Apb1Clock);

  struct Interface * const interface = init(I2CSlave, &i2cSlave0Config);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupI2CSlave1(void)
{
  /* I2C1 is connected to the APB3 bus */
  enablePeriphClock(Apb3Clock);

  struct Interface * const interface = init(I2CSlave, &i2cSlave1Config);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct StreamPackage boardSetupI2S(void)
{
  struct I2SDma * const interface = init(I2SDma, &i2sConfig);
  assert(interface);

  struct Stream * const rxStream = i2sDmaGetInput(interface);
  assert(rxStream);
  struct Stream * const txStream = i2sDmaGetOutput(interface);
  assert(txStream);

  return (struct StreamPackage){
      (struct Interface *)interface,
      rxStream,
      txStream
  };
}
/*----------------------------------------------------------------------------*/
struct PwmPackage boardSetupPwm(bool unified)
{
  struct SctPwmUnit * const timer = init(SctPwmUnit,
      unified ? &pwmTimerUnifiedConfig : &pwmTimerConfig);
  assert(timer);

  struct Pwm * const pwm0 = sctPwmCreate(timer, BOARD_PWM_0);
  assert(pwm0);
  struct Pwm * const pwm1 = sctPwmCreate(timer, BOARD_PWM_1);
  assert(pwm1);
  struct Pwm * const pwm2 = sctPwmCreateDoubleEdge(timer, BOARD_PWM_2);
  assert(pwm2);

  return (struct PwmPackage){
      (struct Timer *)timer,
      pwm0,
      {pwm0, pwm1, pwm2}
  };
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupRit(void)
{
  struct Timer * const timer = init(Rit, 0);
  assert(timer);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct RtClock *boardSetupRtc(bool restart)
{
  /* Override default config */
  struct RtcConfig config = rtcConfig;

  if (restart)
    config.timestamp = 0;

  if (!clockReady(RtcOsc))
  {
    clockEnable(RtcOsc, 0);
    while (!clockReady(RtcOsc));
  }

  struct RtClock * const timer = init(Rtc, &config);
  assert(timer);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSdmmc(bool wide)
{
  assert(clockReady(SystemPll));

  /* Make 51 MHz clock for SDMMC */
  clockEnable(DividerD, &divDConfig);
  while (!clockReady(DividerD));

  clockEnable(SdioClock, &divDClockSource);
  while (!clockReady(SdioClock));

  struct Interface * const interface = init(Sdmmc,
      wide ? &sdmmcConfig4Bit : &sdmmcConfig1Bit);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerial(void)
{
  if (serialConfig.channel == 0)
    enablePeriphClock(Usart0Clock);
  else if (serialConfig.channel == 1)
    enablePeriphClock(Uart1Clock);
  else if (serialConfig.channel == 2)
    enablePeriphClock(Usart2Clock);
  else
    enablePeriphClock(Usart3Clock);

  struct Interface * const interface = init(Serial, &serialConfig);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerialDma(void)
{
  if (serialDmaConfig.channel == 0)
    enablePeriphClock(Usart0Clock);
  else if (serialDmaConfig.channel == 1)
    enablePeriphClock(Uart1Clock);
  else if (serialDmaConfig.channel == 2)
    enablePeriphClock(Usart2Clock);
  else
    enablePeriphClock(Usart3Clock);

  struct Interface * const interface = init(SerialDma, &serialDmaConfig);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpi0(void)
{
  enablePeriphClock(Ssp0Clock);

  struct Interface * const interface = init(Spi, &spi0Config);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpi1(void)
{
  enablePeriphClock(Ssp1Clock);

  struct Interface * const interface = init(Spi, &spi1Config);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpiDma0(void)
{
  enablePeriphClock(Ssp0Clock);

  struct Interface * const interface = init(SpiDma, &spiDma0Config);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpiDma1(void)
{
  enablePeriphClock(Ssp1Clock);

  struct Interface * const interface = init(SpiDma, &spiDma1Config);
  assert(interface);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpifi(void)
{
  static const struct GenericClockConfig source = {
      .source = CLOCK_IDIVA
  };

  /* Maximum possible frequency for SPIFI is 104 MHz */
  struct GenericDividerConfig config;

  if (clockReady(SystemPll))
  {
    config.divisor = 2;
    config.source = CLOCK_PLL;
  }
  else
  {
    config.divisor = 1;
    config.source = clockReady(ExternalOsc) ? CLOCK_EXTERNAL : CLOCK_INTERNAL;
  }

  clockEnable(DividerA, &config);
  while (!clockReady(DividerA));
  clockEnable(SpifiClock, &source);
  while (!clockReady(SpifiClock));

  struct Interface * const interface = init(Spifi, &spifiConfig);
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
struct Entity *boardSetupUsb0(void)
{
  clockEnable(UsbPll, &usbPllConfig);
  while (!clockReady(UsbPll));

  clockEnable(Usb0Clock, &usb0ClockConfig);
  while (!clockReady(Usb0Clock));

  struct Entity * const usb = init(UsbDevice, &usb0Config);
  assert(usb);
  return usb;
}
/*----------------------------------------------------------------------------*/
struct Entity *boardSetupUsb1(void)
{
  /* Make 120 MHz clock on AUDIO PLL */
  if (!clockReady(AudioPll))
  {
    clockEnable(AudioPll, &audioPllConfig);
    while (!clockReady(AudioPll));
  }

  /* Make 60 MHz clock required for USB1 */
  clockEnable(DividerC, &divCConfig);
  while (!clockReady(DividerC));

  clockEnable(Usb1Clock, &divCClockSource);
  while (!clockReady(Usb1Clock));

  struct Entity * const usb = init(UsbDevice, &usb1Config);
  assert(usb);
  return usb;
}
/*----------------------------------------------------------------------------*/
struct Watchdog *boardSetupWdt(void)
{
  struct Watchdog * const timer = init(Wdt, &wdtConfig);
  assert(timer);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Watchdog *boardSetupWwdt(bool disarmed)
{
  /* Override default config */
  struct WwdtConfig config = wwdtConfig;

  if (disarmed)
  {
    config.window = 0;
    config.disarmed = true;
  }

  struct Watchdog * const timer = init(Wwdt, &config);
  assert(timer);
  return timer;
}
