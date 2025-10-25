/*
 * lpc17xx_default/shared/board.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/generic/sdio_spi.h>
#include <halm/platform/lpc/adc.h>
#include <halm/platform/lpc/adc_dma.h>
#include <halm/platform/lpc/adc_dma_stream.h>
#include <halm/platform/lpc/adc_oneshot.h>
#include <halm/platform/lpc/bod.h>
#include <halm/platform/lpc/can.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/dac.h>
#include <halm/platform/lpc/dac_dma.h>
#include <halm/platform/lpc/flash.h>
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
[[gnu::alias("boardSetupTimer1")]] struct Timer *boardSetupAdcTimer(void);
[[gnu::alias("boardSetupTimer0")]] struct Timer *boardSetupTimer(void);
[[gnu::alias("boardSetupTimer1")]] struct Timer *boardSetupTimerAux(void);

[[gnu::alias("boardSetupSpiDma0")]] struct Interface *boardSetupSpiSdio(void);

[[gnu::alias("boardSetupSpi1")]] struct Interface *boardSetupSpi(void);
[[gnu::alias("boardSetupSpiDma1")]] struct Interface *boardSetupSpiDma(void);
/*----------------------------------------------------------------------------*/
const PinNumber adcPinArray[] = {
    PIN(0, 25),
    PIN(1, 31),
    PIN(0, 3),
    PIN(0, 2),
    0
};

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};
/*----------------------------------------------------------------------------*/
DECLARE_WQ_IRQ(WQ_LP, SPI_ISR)
/*----------------------------------------------------------------------------*/
size_t boardGetAdcPinCount(void)
{
  return ARRAY_SIZE(adcPinArray) - 1;
}
/*----------------------------------------------------------------------------*/
void boardSetAdcTimerRate(struct Timer *timer, size_t count, unsigned int rate)
{
  timerSetOverflow(timer, timerGetFrequency(timer) / (count * rate * 2));
}
/*----------------------------------------------------------------------------*/
void boardResetClock(void)
{
  clockEnable(MainClock, &(struct GenericClockConfig){CLOCK_INTERNAL});

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

  clockEnable(MainClock, &(struct GenericClockConfig){CLOCK_EXTERNAL});
}
/*----------------------------------------------------------------------------*/
const struct ClockClass *boardSetupClockOutput(unsigned int divisor)
{
  const struct ClockOutputConfig clockOutputConfig = {
      .divisor = divisor,
      .pin = PIN(1, 27),
      .source = CLOCK_MAIN
  };

  clockEnable(ClockOutput, &clockOutputConfig);
  while (!clockReady(ClockOutput));

  return ClockOutput;
}
/*----------------------------------------------------------------------------*/
void boardSetupClockPll(void)
{
  static const struct PllConfig sysPllConfig = {
      .divisor = 4,
      .multiplier = 32,
      .source = CLOCK_EXTERNAL
  };

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &(struct GenericClockConfig){CLOCK_PLL});
}
/*----------------------------------------------------------------------------*/
void boardSetupLowPriorityWQ(void)
{
  static const struct WorkQueueIrqConfig wqIrqConfig = {
      .size = 4,
      .irq = SPI_IRQ,
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
      .event = ADC_TIMER1_MAT1,
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
      .event = ADC_TIMER1_MAT1,
      .channel = 0,
      .dma = 4
  };

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
      .converter = {ADC_TIMER1_MAT1, 0},
      .memory = {GPDMA_MAT0_0, 1},
      .channel = 0
  };

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
      .eventLevel = BOD_EVENT_2V2,
      .resetLevel = BOD_RESET_DISABLED
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
  const struct CanConfig canConfig = {
      .timer = timer,
      .rate = 100000,
      .rxBuffers = 4,
      .txBuffers = 4,
      .rx = PIN(0, 0),
      .tx = PIN(0, 1),
      .channel = 0
  };

  struct Interface * const interface = init(Can, &canConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct CapturePackage boardSetupCapture(void)
{
  static const struct GpTimerCaptureUnitConfig captureTimerConfig = {
      .frequency = 1000000,
      .channel = 1
  };

  struct GpTimerCaptureUnit * const timer =
      init(GpTimerCaptureUnit, &captureTimerConfig);
  assert(timer != NULL);

  struct Capture * const capture =
      gpTimerCaptureCreate(timer, BOARD_CAPTURE, INPUT_RISING, PIN_PULLDOWN);
  assert(capture != NULL);

  return (struct CapturePackage){(struct Timer *)timer, capture};
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupCounterTimer(void)
{
  static const struct GpTimerCounterConfig counterTimerConfig = {
      .edge = INPUT_RISING,
      .pin = BOARD_CAPTURE,
      .channel = 1
  };

  struct Timer * const timer = init(GpTimerCounter, &counterTimerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupDac(void)
{
  static const struct DacConfig dacConfig = {
      .pin = PIN(0, 26),
      .value = 32768
  };

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
      .pin = PIN(0, 26),
      .dma = 5
  };

  struct DacDma * const interface = init(DacDma, &dacDmaConfig);
  assert(interface != NULL);

  struct Stream * const stream = dacDmaGetOutput(interface);
  assert(stream != NULL);

  return (struct StreamPackage){(struct Interface *)interface, NULL, stream};
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupFlash(void)
{
  struct Interface * const interface = init(Flash, NULL);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupI2C(void)
{
  static const struct I2CConfig i2cConfig = {
      .rate = 100000,
      .scl = PIN(0, 11),
      .sda = PIN(0, 10),
      .channel = 2
  };

  struct Interface * const interface = init(I2C, &i2cConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupI2CSlave(void)
{
  static const struct I2CSlaveConfig i2cSlaveConfig = {
      .size = 16,
      .scl = PIN(0, 11),
      .sda = PIN(0, 10),
      .channel = 2
  };

  struct Interface * const interface = init(I2CSlave, &i2cSlaveConfig);
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
      .rx = {
          .sda = PIN(0, 6),
          .dma = 7
      },
      .tx = {
          .sda = PIN(0, 9),
          .sck = PIN(0, 7),
          .ws = PIN(0, 8),
          .mclk = PIN(4, 29),
          .dma = 6
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
struct PwmPackage boardSetupPwm(bool)
{
  static const struct GpPwmUnitConfig pwmTimerConfig = {
      .frequency = 1000000,
      .resolution = 20000,
      .channel = 0
  };

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
struct RtClock *boardSetupRtc(bool restart)
{
  const struct RtcConfig rtcConfig = {
      /* January 1, 2017, 00:00:00 */
      .timestamp = restart ? 0 : 1483228800
  };

  struct RtClock * const timer = init(Rtc, &rtcConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSdio(bool, struct Timer *timer)
{
  static const size_t SDIO_MAX_BLOCKS = 32768 >> 9; /* RAM size / block size */
  static const uint32_t SDIO_POLL_RATE = 5000;
  static const uint8_t SPI_SDIO_MODE = 3;

  /* Configure helper timer for SDIO status polling */
  if (timer != NULL)
  {
    assert(timerGetFrequency(timer) >= 10 * SDIO_POLL_RATE);
    timerSetOverflow(timer, timerGetFrequency(timer) / SDIO_POLL_RATE);
  }

  struct Interface *sdio;
  struct Interface *spi;
  [[maybe_unused]] enum Result res;

  /* Initialize and start a Work Queue for CRC computation */
  boardSetupLowPriorityWQ();
  wqStart(WQ_LP);

  /* Initialize SPI layer */
  spi = boardSetupSpiSdio();
  assert(spi != NULL);
  res = ifSetParam(spi, IF_SPI_MODE, &SPI_SDIO_MODE);
  assert(res == E_OK);

  /* Initialize SDIO layer */
  const struct SdioSpiConfig sdioSpiConfig = {
      .interface = spi,
      .timer = timer,
      .wq = WQ_LP,
      .blocks = SDIO_MAX_BLOCKS,
      .cs = BOARD_SDIO_CS
  };
  sdio = init(SdioSpi, &sdioSpiConfig);
  assert(sdio != NULL);

  return sdio;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerial(void)
{
  static const struct SerialConfig serialConfig = {
      .rxLength = BOARD_UART_BUFFER,
      .txLength = BOARD_UART_BUFFER,
      .rate = 19200,
      .rx = PIN(0, 16),
      .tx = PIN(0, 15),
      .channel = 1
  };

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
      .rx = PIN(0, 16),
      .tx = PIN(0, 15),
      .channel = 1,
      .dma = {2, 3}
  };

  struct Interface * const interface = init(SerialDma, &serialDmaConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpi0(void)
{
  static const struct SpiConfig spiConfig = {
      .rate = 2000000,
      .miso = PIN(0, 17),
      .mosi = PIN(0, 18),
      .sck = PIN(1, 20),
      .channel = 0,
      .mode = 3
  };

  struct Interface * const interface = init(Spi, &spiConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpi1(void)
{
  static const struct SpiConfig spiConfig = {
      .rate = 2000000,
      .miso = PIN(0, 8),
      .mosi = PIN(0, 9),
      .sck = PIN(0, 7),
      .channel = 1,
      .mode = 3
  };

  struct Interface * const interface = init(Spi, &spiConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpiDma0(void)
{
  static const struct SpiDmaConfig spiDmaConfig = {
      .rate = 2000000,
      .miso = PIN(0, 17),
      .mosi = PIN(0, 18),
      .sck = PIN(1, 20),
      .channel = 0,
      .mode = 3,
      .dma = {0, 1}
  };

  struct Interface * const interface = init(SpiDma, &spiDmaConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpiDma1(void)
{
  static const struct SpiDmaConfig spiDmaConfig = {
      .rate = 2000000,
      .miso = PIN(0, 8),
      .mosi = PIN(0, 9),
      .sck = PIN(0, 7),
      .channel = 1,
      .mode = 3,
      .dma = {0, 1}
  };

  struct Interface * const interface = init(SpiDma, &spiDmaConfig);
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
struct Timer *boardSetupTimer1(void)
{
  static const struct GpTimerConfig timerConfig = {
      .frequency = 1000000,
      .event = GPTIMER_MATCH1, /* Used as an ADC trigger */
      .channel = 1
  };

  struct Timer * const timer = init(GpTimer, &timerConfig);
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
struct Usb *boardSetupUsb(void)
{
  /* Clocks */
  static const struct GenericClockConfig usbClockConfig = {
      .source = CLOCK_USB_PLL
  };
  static const struct PllConfig usbPllConfig = {
      .divisor = 4,
      .multiplier = 16,
      .source = CLOCK_EXTERNAL
  };

  /* Objects */
  static const struct UsbDeviceConfig usbConfig = {
      .dm = PIN(0, 30),
      .dp = PIN(0, 29),
      .connect = PIN(2, 9),
      .vbus = PIN(1, 30),
      .vid = 0x15A2,
      .pid = 0x0044,
      .channel = 0
  };

  clockEnable(UsbPll, &usbPllConfig);
  while (!clockReady(UsbPll));

  clockEnable(UsbClock, &usbClockConfig);
  while (!clockReady(UsbClock));

  struct Usb * const usb = init(UsbDevice, &usbConfig);
  assert(usb != NULL);
  return usb;
}
/*----------------------------------------------------------------------------*/
struct Watchdog *boardSetupWdt(bool)
{
  static const struct WdtConfig wdtConfig = {
      .period = 1000,
      .source = WDT_CLOCK_PCLK
  };

  struct Watchdog * const timer = init(Wdt, &wdtConfig);
  assert(timer != NULL);
  return timer;
}
