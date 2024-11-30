/*
 * lpc43xx_default/shared/board.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/delay.h>
#include <halm/generic/buffering_proxy.h>
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
#include <halm/platform/lpc/gptimer_capture.h>
#include <halm/platform/lpc/gptimer_counter.h>
#include <halm/platform/lpc/i2c.h>
#include <halm/platform/lpc/i2c_slave.h>
#include <halm/platform/lpc/i2s_dma.h>
#include <halm/platform/lpc/lpc43xx/atimer.h>
#include <halm/platform/lpc/lpc43xx/ethernet.h>
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
#include <halm/platform/lpc/wwdt.h>
#include <assert.h>
#include <string.h>
/*----------------------------------------------------------------------------*/
[[gnu::alias("boardSetupTimer3")]] struct Timer *boardSetupAdcTimer(void);
[[gnu::alias("boardSetupTimer0")]] struct Timer *boardSetupTimer(void);

[[gnu::alias("boardSetupCounterTimerGPT")]]
    struct Timer *boardSetupCounterTimer(void);
[[gnu::alias("boardSetupCounterTimerSCTDivided")]]
    struct Timer *boardSetupCounterTimerSCT(void);

[[gnu::alias("boardSetupI2C1")]] struct Interface *boardSetupI2C(void);
[[gnu::alias("boardSetupI2CSlave1")]]
    struct Interface *boardSetupI2CSlave(void);

[[gnu::alias("boardSetupPwmSCT")]] struct PwmPackage boardSetupPwm(bool);
[[gnu::alias("boardSetupPwmSCTDivided")]]
    struct PwmPackage boardSetupPwmSCT(bool);

[[gnu::alias("boardSetupSerial1")]] struct Interface *boardSetupSerial(void);
[[gnu::alias("boardSetupSerialDma1")]]
    struct Interface *boardSetupSerialDma(void);

[[gnu::alias("boardSetupSpi0")]] struct Interface *boardSetupSpi(void);
[[gnu::alias("boardSetupSpiDma0")]] struct Interface *boardSetupSpiDma(void);

[[gnu::alias("boardSetupUsb0")]] struct Usb *boardSetupUsb(void);

static void enablePeriphClock(const void *);
/*----------------------------------------------------------------------------*/
const PinNumber adcPinArray[] = {
    PIN(PORT_ADC, 1),
    PIN(PORT_ADC, 2),
    PIN(PORT_ADC, 6),
    PIN(PORT_ADC, 7),
    0
};

static const struct PllConfig audioPllConfig = {
    .divisor = 4,
    .multiplier = 40,
    .source = CLOCK_EXTERNAL
};

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

[[gnu::section(".shared")]] static struct ClockSettings sharedClockSettings;
/*----------------------------------------------------------------------------*/
static void enablePeriphClock(const void *clock)
{
  if (clockReady(clock))
    clockDisable(clock);

  if (clockReady(SystemPll))
    clockEnable(clock, &(struct GenericClockConfig){CLOCK_PLL});
  else if (clockReady(ExternalOsc))
    clockEnable(clock, &(struct GenericClockConfig){CLOCK_EXTERNAL});
  else
    clockEnable(clock, &(struct GenericClockConfig){CLOCK_INTERNAL});

  while (!clockReady(clock));
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
  bool clockSettingsLoaded = loadClockSettings(&sharedClockSettings);
  const bool spifiClockEnabled = clockReady(SpifiClock);

  if (clockSettingsLoaded)
  {
    /* Check clock sources */
    if (!clockReady(ExternalOsc) || !clockReady(SystemPll))
    {
      memset(&sharedClockSettings, 0, sizeof(sharedClockSettings));
      clockSettingsLoaded = false;
    }
  }

  if (!clockSettingsLoaded)
  {
    clockEnable(MainClock, &(struct GenericClockConfig){CLOCK_INTERNAL});

    if (spifiClockEnabled)
    {
      /* Running from NOR Flash, switch SPIFI clock to IRC without disabling */
      clockEnable(SpifiClock, &(struct GenericClockConfig){CLOCK_INTERNAL});
    }

    clockEnable(ExternalOsc, &extOscConfig);
    while (!clockReady(ExternalOsc));

    clockEnable(MainClock, &(struct GenericClockConfig){CLOCK_EXTERNAL});

    if (spifiClockEnabled)
    {
      /* Switch SPIFI clock to external crystal */
      clockEnable(SpifiClock, &(struct GenericClockConfig){CLOCK_EXTERNAL});
    }

    /* Disable unused System PLL */
    if (clockReady(SystemPll))
      clockDisable(SystemPll);
  }
}
/*----------------------------------------------------------------------------*/
const struct ClockClass *boardSetupClockOutput(uint32_t divisor)
{
  static const struct ClockOutputConfig clockOutputConfig = {
      .pin = PIN(PORT_CLK, 2),
      .source = CLOCK_IDIVE
  };
  const struct GenericDividerConfig divConfig = {
      .divisor = divisor,
      .source = CLOCK_EXTERNAL
  };

  clockEnable(DividerE, &divConfig);
  while (!clockReady(DividerE));

  clockEnable(ClockOutput, &clockOutputConfig);
  while (!clockReady(ClockOutput));

  return ClockOutput;
}
/*----------------------------------------------------------------------------*/
void boardSetupClockPll(void)
{
  static const struct GenericDividerConfig sysDivConfig = {
      .divisor = 2,
      .source = CLOCK_PLL
  };
  static const struct PllConfig sysPllConfig = {
      .divisor = 1,
      .multiplier = 17,
      .source = CLOCK_EXTERNAL
  };

  bool clockSettingsLoaded = loadClockSettings(&sharedClockSettings);
  const bool spifiClockEnabled = clockReady(SpifiClock);

  if (clockSettingsLoaded)
  {
    /* Check clock sources */
    if (!clockReady(ExternalOsc) || !clockReady(SystemPll))
    {
      memset(&sharedClockSettings, 0, sizeof(sharedClockSettings));
      clockSettingsLoaded = false;
    }
  }

  if (!clockSettingsLoaded)
  {
    clockEnable(MainClock, &(struct GenericClockConfig){CLOCK_INTERNAL});

    if (spifiClockEnabled)
    {
      /* Running from NOR Flash, switch SPIFI clock to IRC without disabling */
      clockEnable(SpifiClock, &(struct GenericClockConfig){CLOCK_INTERNAL});
    }

    clockEnable(ExternalOsc, &extOscConfig);
    while (!clockReady(ExternalOsc));

    clockEnable(SystemPll, &sysPllConfig);
    while (!clockReady(SystemPll));

    if (sysPllConfig.divisor == 1)
    {
      /* High frequency, make a PLL clock divided by 2 for base clock ramp up */
      clockEnable(DividerA, &sysDivConfig);
      while (!clockReady(DividerA));

      clockEnable(MainClock, &(struct GenericClockConfig){CLOCK_IDIVA});
      udelay(50);
      clockEnable(MainClock, &(struct GenericClockConfig){CLOCK_PLL});

      /* Base clock is ready, temporary clock divider is not needed anymore */
      clockDisable(DividerA);
    }
    else
    {
      /* Low CPU frequency */
      clockEnable(MainClock, &(struct GenericClockConfig){CLOCK_PLL});
    }
  }

  /* SPIFI */
  if (!clockSettingsLoaded && spifiClockEnabled)
  {
    static const uint32_t spifiMaxFrequency = 30000000;
    const uint32_t frequency = clockFrequency(SystemPll);

    /* Running from NOR Flash, update SPIFI clock without disabling */
    if (frequency > spifiMaxFrequency)
    {
      const struct GenericDividerConfig spifiDivConfig = {
          .divisor = (frequency + spifiMaxFrequency - 1) / spifiMaxFrequency,
          .source = CLOCK_PLL
      };

      clockEnable(DividerD, &spifiDivConfig);
      while (!clockReady(DividerD));

      clockEnable(SpifiClock, &(struct GenericClockConfig){CLOCK_IDIVD});
    }
    else
    {
      clockEnable(SpifiClock, &(struct GenericClockConfig){CLOCK_PLL});
    }
  }
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
      .memory = {GPDMA_SCT_REQ0, 1},
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
      .event = BOARD_BUTTON_INV ? INPUT_FALLING : INPUT_RISING,
      .pull = BOARD_BUTTON_INV ? PIN_PULLUP : PIN_PULLDOWN
  };

  struct Interrupt * const interrupt = init(PinInt, &buttonIntConfig);
  assert(interrupt != NULL);
  return interrupt;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupCan(struct Timer *timer)
{
  /* Clocks */
  static const struct GenericDividerConfig divConfig = {
      .divisor = 3,
      .source = CLOCK_AUDIO_PLL
  };

  /* Objects */
  const struct CanConfig canConfig = {
      .timer = timer,
      .rate = 100000,
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
  clockEnable(DividerA, &divConfig);
  while (!clockReady(DividerA));

  /* Override default config */
  struct CanConfig config = canConfig;
  config.timer = timer;

  if (config.channel == 0)
  {
    /* CAN0 is connected to the APB3 bus */
    clockEnable(Apb3Clock, &(struct GenericClockConfig){CLOCK_IDIVA});
    while (!clockReady(Apb3Clock));
  }
  else
  {
    /* CAN1 is connected to the APB1 bus */
    clockEnable(Apb1Clock, &(struct GenericClockConfig){CLOCK_IDIVA});
    while (!clockReady(Apb1Clock));
  }

  struct Interface * const interface = init(Can, &config);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct CapturePackage boardSetupCapture(void)
{
  static const struct GpTimerCaptureUnitConfig captureTimerConfig = {
      .frequency = 1000000,
      .channel = 2
  };

  struct GpTimerCaptureUnit * const timer =
      init(GpTimerCaptureUnit, &captureTimerConfig);
  assert(timer != NULL);

  struct Capture * const capture =
      gpTimerCaptureCreate(timer, BOARD_CAP_TIMER, INPUT_RISING, PIN_PULLDOWN);
  assert(capture != NULL);

  return (struct CapturePackage){(struct Timer *)timer, capture};
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupCounterTimerGPT(void)
{
  static const struct GpTimerCounterConfig counterTimerConfig = {
      .edge = INPUT_RISING,
      .pin = BOARD_CAP_TIMER,
      .channel = 2
  };

  struct Timer * const timer = init(GpTimerCounter, &counterTimerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupCounterTimerSCTDivided(void)
{
  static const struct SctCounterConfig counterTimerConfig = {
      .pin = BOARD_CAPTURE,
      .edge = INPUT_RISING,
      .part = SCT_LOW,
      .channel = 0
  };

  struct Timer * const timer = init(SctCounter, &counterTimerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupCounterTimerSCTUnified(void)
{
  static const struct SctCounterConfig counterTimerConfig = {
      .pin = BOARD_CAPTURE,
      .edge = INPUT_RISING,
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
struct EthernetPackage boardSetupEthernet(uint64_t address, size_t rxSize,
    size_t txSize)
{
  const struct EthernetConfig ethernetConfig = {
      .address = address,

      .rate = 100000000,
      .rxSize = rxSize,
      .txSize = txSize,
      .halfduplex = false,
      .txclk = PIN(PORT_1, 19),
      .rxd = {
          PIN(PORT_1, 15),
          PIN(PORT_0, 0),
          0,
          0
      },
      .rxdv = PIN(PORT_1, 16),
      .txd = {
          PIN(PORT_1, 18),
          PIN(PORT_1, 20),
          0,
          0
      },
      .txen = PIN(PORT_0, 1),
      .mdc = PIN(PORT_2, 0),
      .mdio = PIN(PORT_1, 17)
  };

  clockEnable(PhyRxClock, &(struct GenericClockConfig){CLOCK_ENET_TX});
  while (!clockReady(PhyRxClock));
  clockEnable(PhyTxClock, &(struct GenericClockConfig){CLOCK_ENET_TX});
  while (!clockReady(PhyTxClock));

  struct Ethernet * const eth = init(Ethernet, &ethernetConfig);
  assert(eth != NULL);

  struct Interface * const mdio = ethMakeMDIO(eth);
  assert(mdio != NULL);

  const struct BufferingProxyConfig proxyConfig = {
      .pipe = eth,
      .rx = {
          .stream = ethGetInput(eth),
          .count = 4,
          .size = BOARD_ETH_BUFFER
      },
      .tx = {
          .stream = ethGetOutput(eth),
          .count = 4,
          .size = BOARD_ETH_BUFFER
      }
  };
  struct Interface * const proxy = init(BufferingProxy, &proxyConfig);
  assert(proxy != NULL);

  return (struct EthernetPackage){
      (struct Interface *)eth,
      mdio,
      proxy
  };
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
      .size = 8,
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

  /* I2S are connected to the APB1 bus */
  enablePeriphClock(Apb1Clock);

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
struct PwmPackage boardSetupPwmSCTDivided(bool centered)
{
  const struct SctPwmUnitConfig pwmTimerConfig = {
      .frequency = 1000000,
      .resolution = 20000,
      .part = SCT_HIGH,
      .channel = 0,
      .centered = centered
  };
  const bool inversion = false;

  struct SctPwmUnit * const timer = init(SctPwmUnit, &pwmTimerConfig);
  assert(timer != NULL);

  struct Pwm * const pwm0 = sctPwmCreate(timer, BOARD_PWM_0, inversion);
  assert(pwm0 != NULL);
  struct Pwm * const pwm1 = sctPwmCreate(timer, BOARD_PWM_1, inversion);
  assert(pwm1 != NULL);
  struct Pwm * const pwm2 = sctPwmCreateDoubleEdge(timer, BOARD_PWM_2,
      inversion);
  assert(pwm2 != NULL);

  return (struct PwmPackage){
      (struct Timer *)timer,
      pwm0,
      {pwm0, pwm1, pwm2}
  };
}
/*----------------------------------------------------------------------------*/
struct PwmPackage boardSetupPwmSCTUnified(bool centered)
{
  const struct SctPwmUnitConfig pwmTimerConfig = {
      .frequency = 1000000,
      .resolution = 20000,
      .part = SCT_UNIFIED,
      .channel = 0,
      .centered = centered
  };
  const bool inversion = false;

  struct SctPwmUnit * const timer = init(SctPwmUnit, &pwmTimerConfig);
  assert(timer != NULL);

  struct Pwm * const pwm0 = sctPwmCreate(timer, BOARD_PWM_0, inversion);
  assert(pwm0 != NULL);
  struct Pwm * const pwm1 = sctPwmCreate(timer, BOARD_PWM_1, inversion);
  assert(pwm1 != NULL);
  struct Pwm * const pwm2 = sctPwmCreateDoubleEdge(timer, BOARD_PWM_2,
      inversion);
  assert(pwm2 != NULL);

  return (struct PwmPackage){
      (struct Timer *)timer,
      pwm0,
      {pwm0, pwm1, pwm2}
  };
}
/*----------------------------------------------------------------------------*/
struct RtClock *boardSetupRtc(bool restart)
{
  if (!clockReady(RtcOsc))
  {
    restart = false;

    clockEnable(RtcOsc, NULL);
    while (!clockReady(RtcOsc));
  }

  const struct RtcConfig rtcConfig = {
      /* January 1, 2017, 00:00:00 */
      .timestamp = restart ? 0 : 1483228800
  };

  /* Non-blocking initialization */
  struct RtClock * const timer = init(Rtc, &rtcConfig);
  assert(timer != NULL);

  /* Wait for RTC registers to update */
  while (rtTime(timer) == 0);

  return timer;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSdio(bool wide, struct Timer *timer)
{
  /* Clocks */
  static const struct GenericDividerConfig divConfig = {
      .divisor = 4,
      .source = CLOCK_PLL
  };

  /* Objects */
  const struct SdmmcConfig sdmmcConfig1Bit = {
      .timer = timer,
      .rate = 1000000,
      .clk = PIN(PORT_CLK, 0),
      .cmd = PIN(PORT_1, 6),
      .dat0 = PIN(PORT_1, 9)
  };
  const struct SdmmcConfig sdmmcConfig4Bit = {
      .timer = timer,
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
  clockEnable(DividerC, &divConfig);
  while (!clockReady(DividerC));

  clockEnable(SdioClock, &(struct GenericClockConfig){CLOCK_IDIVC});
  while (!clockReady(SdioClock));

  struct Interface * const interface = init(Sdmmc,
      wide ? &sdmmcConfig4Bit : &sdmmcConfig1Bit);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerial0(void)
{
  static const struct SerialConfig serialConfig = {
      .rxLength = BOARD_UART_BUFFER,
      .txLength = BOARD_UART_BUFFER,
      .rate = 19200,
      .rx = PIN(PORT_6, 5),
      .tx = PIN(PORT_6, 4),
      .channel = 0
  };

  enablePeriphClock(Usart0Clock);

  struct Interface * const interface = init(Serial, &serialConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerial1(void)
{
  static const struct SerialConfig serialConfig = {
      .rxLength = BOARD_UART_BUFFER,
      .txLength = BOARD_UART_BUFFER,
      .rate = 19200,
      .rx = PIN(PORT_1, 14),
      .tx = PIN(PORT_5, 6),
      .channel = 1
  };

  enablePeriphClock(Uart1Clock);

  struct Interface * const interface = init(Serial, &serialConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerial2(void)
{
  static const struct SerialConfig serialConfig = {
      .rxLength = BOARD_UART_BUFFER,
      .txLength = BOARD_UART_BUFFER,
      .rate = 19200,
      .rx = PIN(PORT_2, 11),
      .tx = PIN(PORT_2, 10),
      .channel = 2
  };

  enablePeriphClock(Usart2Clock);

  struct Interface * const interface = init(Serial, &serialConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerialDma0(void)
{
  static const struct SerialDmaConfig serialDmaConfig = {
      .rxChunks = 4,
      .rxLength = BOARD_UART_BUFFER,
      .txLength = BOARD_UART_BUFFER,
      .rate = 19200,
      .rx = PIN(PORT_6, 5),
      .tx = PIN(PORT_6, 4),
      .channel = 0,
      .dma = {2, 3}
  };

  enablePeriphClock(Usart0Clock);

  struct Interface * const interface = init(SerialDma, &serialDmaConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerialDma1(void)
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

  enablePeriphClock(Uart1Clock);

  struct Interface * const interface = init(SerialDma, &serialDmaConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerialDma2(void)
{
  static const struct SerialDmaConfig serialDmaConfig = {
      .rxChunks = 4,
      .rxLength = BOARD_UART_BUFFER,
      .txLength = BOARD_UART_BUFFER,
      .rate = 19200,
      .rx = PIN(PORT_2, 11),
      .tx = PIN(PORT_2, 10),
      .channel = 2,
      .dma = {2, 3}
  };

  enablePeriphClock(Usart2Clock);

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
struct Interface *boardSetupSpim(struct Timer *)
{
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

  if (clockReady(SystemPll))
  {
    /* Use safe settings, maximum possible frequency for SPIFI is 104 MHz */
    static const uint32_t spifiMaxFrequency = 30000000;
    const uint32_t frequency = clockFrequency(SystemPll);

    if (frequency > spifiMaxFrequency)
    {
      const struct GenericDividerConfig spifiDivConfig = {
          .divisor = (frequency + spifiMaxFrequency - 1) / spifiMaxFrequency,
          .source = CLOCK_PLL
      };

      clockEnable(DividerD, &spifiDivConfig);
      while (!clockReady(DividerD));

      clockEnable(SpifiClock, &(struct GenericClockConfig){CLOCK_IDIVD});
    }
    else
    {
      clockEnable(SpifiClock, &(struct GenericClockConfig){CLOCK_PLL});
    }
  }
  else if (clockReady(ExternalOsc))
  {
    clockEnable(SpifiClock, &(struct GenericClockConfig){CLOCK_EXTERNAL});
    while (!clockReady(SpifiClock));
  }
  else
  {
    clockEnable(SpifiClock, &(struct GenericClockConfig){CLOCK_INTERNAL});
    while (!clockReady(SpifiClock));
  }

  struct Interface * const interface = init(Spifi, &spifiConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimer0(void)
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
struct Timer *boardSetupTimer3(void)
{
  static const struct GpTimerConfig timerConfig = {
      .frequency = 4000000,
      .event = GPTIMER_MATCH3, /* Used as an ADC trigger */
      .channel = 3
  };

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimerAlarm(void)
{
  if (!clockReady(RtcOsc))
  {
    clockEnable(RtcOsc, NULL);
    while (!clockReady(RtcOsc));
  }

  struct Timer * const timer = init(Atimer, NULL);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimerRIT(void)
{
  struct Timer * const timer = init(Rit, NULL);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimerSCT(void)
{
  static const struct SctTimerConfig timerConfig = {
      .frequency = 1000000,
      .part = SCT_UNIFIED,
      .channel = 0
  };

  struct Timer * const timer = init(SctUnifiedTimer, &timerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Usb *boardSetupUsb0(void)
{
  /* Clocks */
  static const struct PllConfig usbPllConfig = {
      .divisor = 1,
      .multiplier = 40,
      .source = CLOCK_EXTERNAL
  };

  /* Objects */
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

  clockEnable(Usb0Clock, &(struct GenericClockConfig){CLOCK_USB_PLL});
  while (!clockReady(Usb0Clock));

  struct Usb * const usb = init(UsbDevice, &usb0Config);
  assert(usb != NULL);
  return usb;
}
/*----------------------------------------------------------------------------*/
struct Usb *boardSetupUsb1(void)
{
  /* Clocks */
  static const struct GenericDividerConfig divConfig = {
      .divisor = 2,
      .source = CLOCK_AUDIO_PLL
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
  clockEnable(DividerA, &divConfig);
  while (!clockReady(DividerA));

  clockEnable(Usb1Clock, &(struct GenericClockConfig){CLOCK_IDIVA});
  while (!clockReady(Usb1Clock));

  struct Usb * const usb = init(UsbDevice, &usb1Config);
  assert(usb != NULL);
  return usb;
}
/*----------------------------------------------------------------------------*/
struct Watchdog *boardSetupWdt(bool disarmed)
{
  const struct WwdtConfig wwdtConfig = {
      .period = 5000,
      .window = 0,
      .disarmed = disarmed
  };

  struct Watchdog * const timer = init(Wwdt, &wwdtConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Watchdog *boardSetupWwdt(void)
{
  const struct WwdtConfig wwdtConfig = {
      .period = 5000,
      .window = 1000,
      .disarmed = false
  };

  struct Watchdog * const timer = init(Wwdt, &wwdtConfig);
  assert(timer != NULL);
  return timer;
}
