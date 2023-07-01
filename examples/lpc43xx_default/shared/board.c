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
#include <halm/platform/lpc/bod.h>
#include <halm/platform/lpc/can.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/dac.h>
#include <halm/platform/lpc/dac_dma.h>
#include <halm/platform/lpc/eeprom.h>
#include <halm/platform/lpc/flash.h>
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
#include <halm/platform/lpc/sct_timer.h>
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

struct Entity *boardSetupUsb(void)
    __attribute__((alias("boardSetupUsb0")));

static void enablePeriphClock(const void *);
/*----------------------------------------------------------------------------*/
const PinNumber adcPinArray[] = {
    PIN(PORT_ADC, 1),
    PIN(PORT_ADC, 2),
    PIN(PORT_ADC, 3),
    PIN(PORT_ADC, 5),
    0
};

static const struct PllConfig audioPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 4,
    .multiplier = 40
};

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};
/*----------------------------------------------------------------------------*/
static void enablePeriphClock(const void *clock)
{
  if (!clockReady(clock))
  {
    if (clockReady(SystemPll))
      clockEnable(clock, &(struct GenericClockConfig){CLOCK_PLL});
    else if (clockReady(ExternalOsc))
      clockEnable(clock, &(struct GenericClockConfig){CLOCK_EXTERNAL});
    else
      clockEnable(clock, &(struct GenericClockConfig){CLOCK_INTERNAL});

    while (!clockReady(clock));
  }
}
/*----------------------------------------------------------------------------*/
size_t boardGetAdcPinCount(void)
{
  return ARRAY_SIZE(adcPinArray) - 1;
}
/*----------------------------------------------------------------------------*/
void boardSetAdcTimerRate(struct Timer *timer, size_t count, uint32_t rate)
{
  timerSetOverflow(timer, timerGetFrequency(timer) / (count * rate * 2));
}
/*----------------------------------------------------------------------------*/
void boardResetClock(void)
{
  clockEnable(MainClock, &(struct GenericClockConfig){CLOCK_INTERNAL});

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
  clockEnable(MainClock, &(struct GenericClockConfig){CLOCK_INTERNAL});

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainClock, &(struct GenericClockConfig){CLOCK_EXTERNAL});
}
/*----------------------------------------------------------------------------*/
void boardSetupClockInt(void)
{
  clockEnable(MainClock, &(struct GenericClockConfig){CLOCK_INTERNAL});
}
/*----------------------------------------------------------------------------*/
const struct ClockClass *boardSetupClockOutput(uint32_t divisor)
{
  static const struct ClockOutputConfig clockOutputConfig = {
      .source = CLOCK_IDIVE,
      .pin = PIN(PORT_CLK, 2)
  };
  const struct GenericDividerConfig divEConfig = {
      .source = CLOCK_EXTERNAL,
      .divisor = divisor
  };

  clockEnable(DividerE, &divEConfig);
  while (!clockReady(DividerE));

  clockEnable(ClockOutput, &clockOutputConfig);
  while (!clockReady(ClockOutput));

  return ClockOutput;
}
/*----------------------------------------------------------------------------*/
void boardSetupClockPll(void)
{
  static const struct PllConfig systemPllConfig = {
      .source = CLOCK_EXTERNAL,
      .divisor = 2,
      .multiplier = 17
  };

  clockEnable(MainClock, &(struct GenericClockConfig){CLOCK_INTERNAL});

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &systemPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &(struct GenericClockConfig){CLOCK_PLL});
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupAdc(void)
{
  static const struct AdcConfig adcConfig = {
      .pins = adcPinArray,
      .event = ADC_CTOUT_15,
      .channel = 0
  };

  /* ADC0 and ADC1 are connected to the APB3 bus */
  enablePeriphClock(Apb3Clock);

  struct Interface * const interface = init(Adc, &adcConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupAdcDma(void)
{
  static const struct AdcDmaConfig adcDmaConfig = {
      .pins = adcPinArray,
      .event = ADC_CTOUT_15,
      .channel = 0,
      .dma = 4
  };

  /* ADC0 and ADC1 are connected to the APB3 bus */
  enablePeriphClock(Apb3Clock);

  struct Interface * const interface = init(AdcDma, &adcDmaConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupAdcOneShot(void)
{
  const struct AdcOneShotConfig adcOneShotConfig = {
      .pin = adcPinArray[0],
      .channel = 0
  };

  /* ADC0 and ADC1 are connected to the APB3 bus */
  enablePeriphClock(Apb3Clock);

  struct Interface * const interface = init(AdcOneShot, &adcOneShotConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct StreamPackage boardSetupAdcStream(void)
{
  static const struct AdcDmaStreamConfig adcStreamConfig = {
      .pins = adcPinArray,
      .size = 2,
      .converter = {ADC_CTOUT_15, 0},
      .memory = {GPDMA_MAT0_0, 1},
      .channel = 0
  };

  /* ADC0 and ADC1 are connected to the APB3 bus */
  enablePeriphClock(Apb3Clock);

  struct AdcDmaStream * const interface = init(AdcDmaStream, &adcStreamConfig);
  assert(interface != NULL);

  struct Stream * const stream = adcDmaStreamGetInput(interface);
  assert(stream != NULL);

  return (struct StreamPackage){(struct Interface *)interface, stream, NULL};
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupAdcTimer(void)
{
  static const struct GpTimerConfig adcTimerConfig = {
      .frequency = 4000000,
      .event = GPTIMER_MATCH3,
      .channel = 3
  };

  struct Timer * const timer = init(GpTimer, &adcTimerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Interrupt *boardSetupBod(void)
{
  static const struct BodConfig bodConfig = {
      .eventLevel = BOD_EVENT_3V05,
      .resetLevel = BOD_RESET_2V1
  };

  struct Interrupt * const interrupt = init(Bod, &bodConfig);
  assert(interrupt != NULL);
  return interrupt;
}
/*----------------------------------------------------------------------------*/
struct Interrupt *boardSetupButton(void)
{
  static const struct PinIntConfig buttonIntConfig = {
      .pin = BOARD_BUTTON,
      .event = PIN_FALLING,
      .pull = PIN_PULLUP
  };

  struct Interrupt * const interrupt = init(PinInt, &buttonIntConfig);
  assert(interrupt != NULL);
  return interrupt;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupCan(struct Timer *timer)
{
  /* Clocks */
  static const struct GenericDividerConfig divBConfig = {
      .source = CLOCK_AUDIO_PLL,
      .divisor = 3
  };

  /* Objects */
  const struct CanConfig canConfig = {
      .timer = timer,
      .rate = 1000000,
      .rxBuffers = 4,
      .txBuffers = 4,
      .rx = PIN(PORT_3, 1),
      .tx = PIN(PORT_3, 2),
      .channel = 0
  };

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
    clockEnable(Apb3Clock, &(struct GenericClockConfig){CLOCK_IDIVB});
    while (!clockReady(Apb3Clock));
  }
  else
  {
    /* CAN1 is connected to the APB1 bus */
    clockEnable(Apb1Clock, &(struct GenericClockConfig){CLOCK_IDIVB});
    while (!clockReady(Apb1Clock));
  }

  struct Interface * const interface = init(Can, &config);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupCounterTimer(void)
{
  static const struct SctCounterConfig counterTimerConfig = {
      .pin = BOARD_CAPTURE,
      .edge = PIN_RISING,
      .part = SCT_UNIFIED,
      .channel = 0
  };

  struct Timer * const timer = init(SctUnifiedCounter, &counterTimerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupDac(void)
{
  static const struct DacConfig dacConfig = {
      .pin = PIN(PORT_4, 4),
      .value = 32768
  };

  /* DAC is connected to the APB3 bus */
  enablePeriphClock(Apb3Clock);

  struct Interface * const interface = init(Dac, &dacConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct StreamPackage boardSetupDacDma(void)
{
  static const struct DacDmaConfig dacDmaConfig = {
      .size = 2,
      .rate = 96000,
      .value = 32768,
      .pin = PIN(PORT_4, 4),
      .dma = 5
  };

  /* DAC is connected to the APB3 bus */
  enablePeriphClock(Apb3Clock);

  struct DacDma * const interface = init(DacDma, &dacDmaConfig);
  assert(interface != NULL);

  struct Stream * const stream = dacDmaGetOutput(interface);
  assert(stream != NULL);

  return (struct StreamPackage){(struct Interface *)interface, NULL, stream};
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupEeprom(void)
{
  struct Interface * const interface = init(Eeprom, NULL);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupFlash(void)
{
  static const struct FlashConfig flashConfig = {
      .bank = FLASH_BANK_A
  };

  struct Interface * const interface = init(Flash, &flashConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupI2C0(void)
{
  static const struct I2CConfig i2c0Config = {
      .rate = 100000,
      .scl = PIN(PORT_I2C, PIN_I2C0_SCL),
      .sda = PIN(PORT_I2C, PIN_I2C0_SDA),
      .channel = 0
  };

  /* I2C0 is connected to the APB1 bus */
  enablePeriphClock(Apb1Clock);

  struct Interface * const interface = init(I2C, &i2c0Config);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupI2C1(void)
{
  static const struct I2CConfig i2c1Config = {
      .rate = 100000,
      .scl = PIN(PORT_2, 4),
      .sda = PIN(PORT_2, 3),
      .channel = 1
  };

  /* I2C1 is connected to the APB3 bus */
  enablePeriphClock(Apb3Clock);

  struct Interface * const interface = init(I2C, &i2c1Config);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupI2CSlave0(void)
{
  static const struct I2CSlaveConfig i2cSlave0Config = {
      .size = 16,
      .scl = PIN(PORT_I2C, PIN_I2C0_SCL),
      .sda = PIN(PORT_I2C, PIN_I2C0_SDA),
      .channel = 0
  };

  /* I2C0 is connected to the APB1 bus */
  enablePeriphClock(Apb1Clock);

  struct Interface * const interface = init(I2CSlave, &i2cSlave0Config);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupI2CSlave1(void)
{
  static const struct I2CSlaveConfig i2cSlave1Config = {
      .size = 16,
      .scl = PIN(PORT_2, 4),
      .sda = PIN(PORT_2, 3),
      .channel = 1
  };

  /* I2C1 is connected to the APB3 bus */
  enablePeriphClock(Apb3Clock);

  struct Interface * const interface = init(I2CSlave, &i2cSlave1Config);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct StreamPackage boardSetupI2S(void)
{
  static const struct I2SDmaConfig i2sConfig = {
      .size = 2,
      .rate = 96000,
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

  struct I2SDma * const interface = init(I2SDma, &i2sConfig);
  assert(interface != NULL);

  struct Stream * const rxStream = i2sDmaGetInput(interface);
  assert(rxStream != NULL);
  struct Stream * const txStream = i2sDmaGetOutput(interface);
  assert(txStream != NULL);

  return (struct StreamPackage){
      (struct Interface *)interface,
      rxStream,
      txStream
  };
}
/*----------------------------------------------------------------------------*/
struct PwmPackage boardSetupPwm(bool unified)
{
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

  struct SctPwmUnit * const timer = init(SctPwmUnit,
      unified ? &pwmTimerUnifiedConfig : &pwmTimerConfig);
  assert(timer != NULL);

  struct Pwm * const pwm0 = sctPwmCreate(timer, BOARD_PWM_0);
  assert(pwm0 != NULL);
  struct Pwm * const pwm1 = sctPwmCreate(timer, BOARD_PWM_1);
  assert(pwm1 != NULL);
  struct Pwm * const pwm2 = sctPwmCreateDoubleEdge(timer, BOARD_PWM_2);
  assert(pwm2 != NULL);

  return (struct PwmPackage){
      (struct Timer *)timer,
      pwm0,
      {pwm0, pwm1, pwm2}
  };
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupRit(void)
{
  struct Timer * const timer = init(Rit, NULL);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct RtClock *boardSetupRtc(bool restart)
{
  const struct RtcConfig rtcConfig = {
      /* January 1, 2017, 00:00:00 */
      .timestamp = restart ? 0 : 1483228800
  };

  if (!clockReady(RtcOsc))
  {
    clockEnable(RtcOsc, NULL);
    while (!clockReady(RtcOsc));
  }

  struct RtClock * const timer = init(Rtc, &rtcConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSdio(bool wide)
{
  /* Clocks */
  static const struct GenericDividerConfig divDConfig = {
      .source = CLOCK_PLL,
      .divisor = 2
  };

  /* Objects */
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

  assert(clockReady(SystemPll));

  /* Make 51 MHz clock for SDMMC */
  clockEnable(DividerD, &divDConfig);
  while (!clockReady(DividerD));

  clockEnable(SdioClock, &(struct GenericClockConfig){CLOCK_IDIVD});
  while (!clockReady(SdioClock));

  struct Interface * const interface = init(Sdmmc,
      wide ? &sdmmcConfig4Bit : &sdmmcConfig1Bit);
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
      .rx = PIN(PORT_1, 14),
      .tx = PIN(PORT_5, 6),
      .channel = 1
  };

  if (serialConfig.channel == 0)
    enablePeriphClock(Usart0Clock);
  else if (serialConfig.channel == 1)
    enablePeriphClock(Uart1Clock);
  else if (serialConfig.channel == 2)
    enablePeriphClock(Usart2Clock);
  else
    enablePeriphClock(Usart3Clock);

  struct Interface * const interface = init(Serial, &serialConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerialDma(void)
{
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

  if (serialDmaConfig.channel == 0)
    enablePeriphClock(Usart0Clock);
  else if (serialDmaConfig.channel == 1)
    enablePeriphClock(Uart1Clock);
  else if (serialDmaConfig.channel == 2)
    enablePeriphClock(Usart2Clock);
  else
    enablePeriphClock(Usart3Clock);

  struct Interface * const interface = init(SerialDma, &serialDmaConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpi0(void)
{
  static const struct SpiConfig spi0Config = {
      .rate = 2000000,
      .sck = PIN(PORT_3, 0),
      .miso = PIN(PORT_1, 1),
      .mosi = PIN(PORT_1, 2),
      .channel = 0,
      .mode = 0
  };

  enablePeriphClock(Ssp0Clock);

  struct Interface * const interface = init(Spi, &spi0Config);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpi1(void)
{
  static const struct SpiConfig spi1Config = {
      .rate = 2000000,
      .sck = PIN(PORT_F, 4),
      .miso = PIN(PORT_1, 3),
      .mosi = PIN(PORT_1, 4),
      .channel = 1,
      .mode = 0
  };

  enablePeriphClock(Ssp1Clock);

  struct Interface * const interface = init(Spi, &spi1Config);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpiDma0(void)
{
  static const struct SpiDmaConfig spiDma0Config = {
      .rate = 2000000,
      .sck = PIN(PORT_3, 0),
      .miso = PIN(PORT_1, 1),
      .mosi = PIN(PORT_1, 2),
      .channel = 0,
      .mode = 0,
      .dma = {0, 1}
  };

  enablePeriphClock(Ssp0Clock);

  struct Interface * const interface = init(SpiDma, &spiDma0Config);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpiDma1(void)
{
  static const struct SpiDmaConfig spiDma1Config = {
      .rate = 2000000,
      .sck = PIN(PORT_F, 4),
      .miso = PIN(PORT_1, 3),
      .mosi = PIN(PORT_1, 4),
      .channel = 1,
      .mode = 0,
      .dma = {0, 1}
  };

  enablePeriphClock(Ssp1Clock);

  struct Interface * const interface = init(SpiDma, &spiDma1Config);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpifi(void)
{
  /* Clocks */
  static const struct GenericClockConfig source = {
      .source = CLOCK_IDIVA
  };

  /* Objects */
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
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimer(void)
{
  static const struct GpTimerConfig timerConfig = {
      .frequency = 1000000,
      .event = GPTIMER_MATCH0,
      .channel = 0
  };

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Entity *boardSetupUsb0(void)
{
  /* Clocks */
  static const struct GenericClockConfig usb0ClockConfig = {
      .source = CLOCK_USB_PLL
  };
  static const struct PllConfig usbPllConfig = {
      .source = CLOCK_EXTERNAL,
      .divisor = 1,
      .multiplier = 40
  };

  /* Objetcs */
  static const struct UsbDeviceConfig usb0Config = {
      .dm = PIN(PORT_USB, PIN_USB0_DM),
      .dp = PIN(PORT_USB, PIN_USB0_DP),
      .connect = 0,
      .vbus = PIN(PORT_USB, PIN_USB0_VBUS),
      .vid = 0x15A2,
      .pid = 0x0044,
      .channel = 0
  };

  clockEnable(UsbPll, &usbPllConfig);
  while (!clockReady(UsbPll));

  clockEnable(Usb0Clock, &usb0ClockConfig);
  while (!clockReady(Usb0Clock));

  struct Entity * const usb = init(UsbDevice, &usb0Config);
  assert(usb != NULL);
  return usb;
}
/*----------------------------------------------------------------------------*/
struct Entity *boardSetupUsb1(void)
{
  /* Clocks */
  static const struct GenericDividerConfig divCConfig = {
      .source = CLOCK_AUDIO_PLL,
      .divisor = 2
  };

  /* Objects */
  static const struct UsbDeviceConfig usb1Config = {
      .dm = PIN(PORT_USB, PIN_USB1_DM),
      .dp = PIN(PORT_USB, PIN_USB1_DP),
      .connect = 0,
      .vbus = PIN(PORT_2, 5),
      .vid = 0x15A2,
      .pid = 0x0044,
      .channel = 1
  };

  /* Make 120 MHz clock on AUDIO PLL */
  if (!clockReady(AudioPll))
  {
    clockEnable(AudioPll, &audioPllConfig);
    while (!clockReady(AudioPll));
  }

  /* Make 60 MHz clock required for USB1 */
  clockEnable(DividerC, &divCConfig);
  while (!clockReady(DividerC));

  clockEnable(Usb1Clock, &(struct GenericClockConfig){CLOCK_IDIVC});
  while (!clockReady(Usb1Clock));

  struct Entity * const usb = init(UsbDevice, &usb1Config);
  assert(usb != NULL);
  return usb;
}
/*----------------------------------------------------------------------------*/
struct Watchdog *boardSetupWdt(bool disarmed __attribute__((unused)))
{
  static const struct WdtConfig wdtConfig = {
      .period = 5000
  };

  struct Watchdog * const timer = init(Wdt, &wdtConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Watchdog *boardSetupWwdt(bool disarmed)
{
  const struct WwdtConfig wwdtConfig = {
      .period = 5000,
      .window = disarmed ? 0 : 1000,
      .disarmed = disarmed
  };

  struct Watchdog * const timer = init(Wwdt, &wwdtConfig);
  assert(timer != NULL);
  return timer;
}
