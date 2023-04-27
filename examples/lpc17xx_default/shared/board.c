/*
 * lpc17xx_default/shared/board.c
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
#include <halm/platform/lpc/gppwm.h>
#include <halm/platform/lpc/gptimer.h>
#include <halm/platform/lpc/gptimer_capture.h>
#include <halm/platform/lpc/gptimer_counter.h>
#include <halm/platform/lpc/i2c.h>
#include <halm/platform/lpc/i2c_slave.h>
#include <halm/platform/lpc/i2s_dma.h>
#include <halm/platform/lpc/pin_int.h>
#include <halm/platform/lpc/rit.h>
#include <halm/platform/lpc/rtc.h>
#include <halm/platform/lpc/serial.h>
#include <halm/platform/lpc/serial_dma.h>
#include <halm/platform/lpc/spi.h>
#include <halm/platform/lpc/spi_dma.h>
#include <halm/platform/lpc/usb_device.h>
#include <halm/platform/lpc/wdt.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSdioSpi(void)
    __attribute__((alias("boardSetupSpi0")));
struct Interface *boardSetupSdioSpiDma(void)
    __attribute__((alias("boardSetupSpiDma0")));

struct Interface *boardSetupSpi(void)
    __attribute__((alias("boardSetupSpi1")));
struct Interface *boardSetupSpiDma(void)
    __attribute__((alias("boardSetupSpiDma1")));
/*----------------------------------------------------------------------------*/
const PinNumber adcPinArray[] = {
    PIN(0, 25),
    PIN(1, 31),
    PIN(0, 3),
    PIN(0, 2),
    0
};

static const struct AdcConfig adcConfig = {
    .pins = adcPinArray,
    .event = ADC_TIMER1_MAT1,
    .channel = 0
};

static const struct AdcDmaConfig adcDmaConfig = {
    .pins = adcPinArray,
    .event = ADC_TIMER1_MAT1,
    .channel = 0,
    .dma = 4
};

static const struct AdcDmaStreamConfig adcStreamConfig = {
    .pins = adcPinArray,
    .size = 2,
    .converter = {ADC_TIMER1_MAT1, 0},
    .memory = {GPDMA_MAT0_0, 1},
    .channel = 0
};

static const struct AdcOneShotConfig adcOneShotConfig = {
    .pin = PIN(0, 25),
    .channel = 0
};

static const struct GpTimerConfig adcTimerConfig = {
    .frequency = 1000000,
    .event = GPTIMER_MATCH1,
    .channel = 1
};

static const struct PinIntConfig buttonIntConfig = {
    .pin = BOARD_BUTTON,
    .event = PIN_FALLING,
    .pull = PIN_PULLUP
};

static const struct CanConfig canConfig = {
    .rate = 1000000,
    .rxBuffers = 4,
    .txBuffers = 4,
    .rx = PIN(0, 0),
    .tx = PIN(0, 1),
    .channel = 0
};

static const struct GpTimerCaptureUnitConfig captureTimerConfig = {
    .frequency = 1000000,
    .channel = 1
};

static const struct GpTimerCounterConfig counterTimerConfig = {
    .edge = PIN_RISING,
    .pin = BOARD_CAPTURE,
    .channel = 1
};

static const struct DacConfig dacConfig = {
    .pin = PIN(0, 26),
    .value = 32768
};

static const struct DacDmaConfig dacDmaConfig = {
    .size = 2,
    .rate = 96000,
    .value = 32768,
    .pin = PIN(0, 26),
    .dma = 5
};

static const struct I2CConfig i2cConfig = {
    .rate = 100000,
    .scl = PIN(0, 11),
    .sda = PIN(0, 10),
    .channel = 2
};

static const struct I2CSlaveConfig i2cSlaveConfig = {
    .size = 16,
    .scl = PIN(0, 11),
    .sda = PIN(0, 10),
    .channel = 2
};

static const struct I2SDmaConfig i2sConfig = {
    .size = 2,
    .rate = 44100,
    .width = I2S_WIDTH_16,
    .tx = {
        .sda = PIN(0, 9),
        .sck = PIN(0, 7),
        .ws = PIN(0, 8),
        .mclk = PIN(4, 29),
        .dma = 6
    },
    .rx = {
        .sda = PIN(0, 6),
        .dma = 7
    },
    .channel = 0,
    .mono = false,
    .slave = false
};

static const struct GpPwmUnitConfig pwmTimerConfig = {
    .frequency = 1000000,
    .resolution = 20000,
    .channel = 0
};

static const struct RtcConfig rtcConfig = {
    /* January 1, 2017, 00:00:00 */
    .timestamp = 1483228800
};

static const struct SerialConfig serialConfig = {
    .rxLength = BOARD_UART_BUFFER,
    .txLength = BOARD_UART_BUFFER,
    .rate = 19200,
    .rx = PIN(0, 16),
    .tx = PIN(0, 15),
    .channel = 1
};

static const struct SerialDmaConfig serialDmaConfig = {
    .rxChunks = 4,
    .rxLength = BOARD_UART_BUFFER,
    .txLength = BOARD_UART_BUFFER,
    .rate = 19200,
    .rx = PIN(0, 16),
    .tx = PIN(0, 15),
    .channel = 1,
    .dma = {2, 3}
};

static const struct SpiConfig spi0Config = {
    .rate = 2000000,
    .miso = PIN(0, 17),
    .mosi = PIN(0, 18),
    .sck = PIN(1, 20),
    .channel = 0,
    .mode = 3
};

static const struct SpiConfig spi1Config = {
    .rate = 2000000,
    .miso = PIN(0, 8),
    .mosi = PIN(0, 9),
    .sck = PIN(0, 7),
    .channel = 1,
    .mode = 3
};

static const struct SpiDmaConfig spiDma0Config = {
    .rate = 2000000,
    .miso = PIN(0, 17),
    .mosi = PIN(0, 18),
    .sck = PIN(1, 20),
    .channel = 0,
    .mode = 3,
    .dma = {0, 1}
};

static const struct SpiDmaConfig spiDma1Config = {
    .rate = 2000000,
    .miso = PIN(0, 8),
    .mosi = PIN(0, 9),
    .sck = PIN(0, 7),
    .channel = 1,
    .mode = 3,
    .dma = {0, 1}
};

static const struct GpTimerConfig timerConfig = {
    .frequency = 1000000,
    .event = GPTIMER_MATCH0,
    .channel = 0
};

static const struct UsbDeviceConfig usbConfig = {
    .dm = PIN(0, 30),
    .dp = PIN(0, 29),
    .connect = PIN(2, 9),
    .vbus = PIN(1, 30),
    .vid = 0x15A2,
    .pid = 0x0044,
    .channel = 0
};

static const struct WdtConfig wdtConfig = {
    .period = 1000,
    .source = WDT_CLOCK_PCLK
};
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct PllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 4,
    .multiplier = 32
};

static const struct PllConfig usbPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 4,
    .multiplier = 16
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

static const struct GenericClockConfig usbClockConfig = {
    .source = CLOCK_USB_PLL
};
/*----------------------------------------------------------------------------*/
size_t boardGetAdcPinCount(void)
{
  return ARRAY_SIZE(adcPinArray) - 1;
}
/*----------------------------------------------------------------------------*/
void boardResetClock(void)
{
  clockEnable(MainClock, &mainClockConfigInt);

  if (clockReady(UsbPll))
    clockDisable(UsbPll);

  if (clockReady(SystemPll))
    clockDisable(SystemPll);

  if (clockReady(ExternalOsc))
    clockDisable(ExternalOsc);
}
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainClock, &mainClockConfigExt);
}
/*----------------------------------------------------------------------------*/
void boardSetupClockPll(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &mainClockConfigPll);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupAdc(void)
{
  struct Interface * const interface = init(Adc, &adcConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupAdcDma(void)
{
  struct Interface * const interface = init(AdcDma, &adcDmaConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupAdcOneShot(void)
{
  struct Interface * const interface = init(AdcOneShot, &adcOneShotConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct StreamPackage boardSetupAdcStream(void)
{
  struct AdcDmaStream * const interface = init(AdcDmaStream, &adcStreamConfig);
  assert(interface != NULL);

  struct Stream * const stream = adcDmaStreamGetInput(interface);
  assert(stream != NULL);

  return (struct StreamPackage){(struct Interface *)interface, stream, NULL};
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupAdcTimer(void)
{
  struct Timer * const timer = init(GpTimer, &adcTimerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Interrupt *boardSetupButton(void)
{
  struct Interrupt * const interrupt = init(PinInt, &buttonIntConfig);
  assert(interrupt != NULL);
  return interrupt;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupCan(struct Timer *timer)
{
  /* Override default config */
  struct CanConfig config = canConfig;
  config.timer = timer;

  struct Interface * const interface = init(Can, &config);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct CapturePackage boardSetupCapture(void)
{
  struct GpTimerCaptureUnit * const timer =
      init(GpTimerCaptureUnit, &captureTimerConfig);
  assert(timer != NULL);

  struct Capture * const capture =
      gpTimerCaptureCreate(timer, BOARD_CAPTURE, PIN_RISING, PIN_PULLDOWN);
  assert(capture != NULL);

  return (struct CapturePackage){(struct Timer *)timer, capture};
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupCounterTimer(void)
{
  struct Timer * const timer = init(GpTimerCounter, &counterTimerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupDac(void)
{
  struct Interface * const interface = init(Dac, &dacConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct StreamPackage boardSetupDacDma(void)
{
  struct DacDma * const interface = init(DacDma, &dacDmaConfig);
  assert(interface != NULL);

  struct Stream * const stream = dacDmaGetOutput(interface);
  assert(stream != NULL);

  return (struct StreamPackage){(struct Interface *)interface, NULL, stream};
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupI2C(void)
{
  struct Interface * const interface = init(I2C, &i2cConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupI2CSlave(void)
{
  struct Interface * const interface = init(I2CSlave, &i2cSlaveConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct StreamPackage boardSetupI2S(void)
{
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
struct PwmPackage boardSetupPwm(void)
{
  struct GpPwmUnit * const timer = init(GpPwmUnit, &pwmTimerConfig);
  assert(timer != NULL);

  struct Pwm * const pwm0 = gpPwmCreate(timer, BOARD_PWM_0);
  assert(pwm0 != NULL);
  struct Pwm * const pwm1 = gpPwmCreate(timer, BOARD_PWM_1);
  assert(pwm1 != NULL);
  struct Pwm * const pwm2 = gpPwmCreateDoubleEdge(timer, BOARD_PWM_2);
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
  /* Override default config */
  struct RtcConfig config = rtcConfig;

  if (restart)
    config.timestamp = 0;

  struct RtClock * const timer = init(Rtc, &config);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerial(void)
{
  struct Interface * const interface = init(Serial, &serialConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerialDma(void)
{
  struct Interface * const interface = init(SerialDma, &serialDmaConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpi0(void)
{
  struct Interface * const interface = init(Spi, &spi0Config);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpi1(void)
{
  struct Interface * const interface = init(Spi, &spi1Config);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpiDma0(void)
{
  struct Interface * const interface = init(SpiDma, &spiDma0Config);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpiDma1(void)
{
  struct Interface * const interface = init(SpiDma, &spiDma1Config);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimer(void)
{
  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Entity *boardSetupUsb(void)
{
  clockEnable(UsbPll, &usbPllConfig);
  while (!clockReady(UsbPll));

  clockEnable(UsbClock, &usbClockConfig);
  while (!clockReady(UsbClock));

  struct Entity * const usb = init(UsbDevice, &usbConfig);
  assert(usb != NULL);
  return usb;
}
/*----------------------------------------------------------------------------*/
struct Watchdog *boardSetupWdt(void)
{
  struct Watchdog * const timer = init(Wdt, &wdtConfig);
  assert(timer != NULL);
  return timer;
}
