/*
 * lpc13uxx_default/shared/board.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/lpc/adc.h>
#include <halm/platform/lpc/adc_oneshot.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/gptimer.h>
#include <halm/platform/lpc/gptimer_capture.h>
#include <halm/platform/lpc/gptimer_counter.h>
#include <halm/platform/lpc/gptimer_pwm.h>
#include <halm/platform/lpc/i2c.h>
#include <halm/platform/lpc/pin_int.h>
#include <halm/platform/lpc/serial.h>
#include <halm/platform/lpc/spi.h>
#include <halm/platform/lpc/usb_device.h>
#include <halm/platform/lpc/wdt.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
const PinNumber adcPinArray[] = {
    PIN(0, 11),
    PIN(0, 23),
    0
};

static const struct AdcConfig adcConfig = {
    .pins = adcPinArray,
    .event = ADC_CT32B0_MAT0,
    .channel = 0
};

static const struct AdcOneShotConfig adcOneShotConfig = {
    .pin = PIN(0, 11),
    .channel = 0
};

static const struct GpTimerConfig adcTimerConfig = {
    .frequency = 1000000,
    .event = GPTIMER_MATCH0,
    .channel = GPTIMER_CT32B0
};

static const struct PinIntConfig buttonIntConfig = {
    .pin = BOARD_BUTTON,
    .event = PIN_FALLING,
    .pull = PIN_PULLUP
};

static const struct GpTimerCaptureUnitConfig captureTimerConfig = {
    .frequency = 1000000,
    .channel = GPTIMER_CT32B0
};

static const struct ClockOutputConfig clockOutputConfig = {
    .source = CLOCK_MAIN,
    .divisor = 0,
    .pin = PIN(0, 1)
};

static const struct GpTimerCounterConfig counterTimerConfig = {
    .edge = PIN_RISING,
    .pin = BOARD_CAPTURE,
    .channel = GPTIMER_CT32B0
};

static const struct I2CConfig i2cConfig = {
    .rate = 100000,
    .scl = PIN(0, 4),
    .sda = PIN(0, 5),
    .channel = 0
};

static const struct GpTimerPwmUnitConfig pwmTimerConfig = {
    .frequency = 1000000,
    .resolution = 20000,
    .channel = GPTIMER_CT16B1
};

static const struct SerialConfig serialConfig = {
    .rxLength = BOARD_UART_BUFFER,
    .txLength = BOARD_UART_BUFFER,
    .rate = 19200,
    .rx = PIN(0, 18),
    .tx = PIN(0, 19),
    .channel = 0
};

static const struct SpiConfig spiConfig = {
    .rate = 2000000,
    .miso = PIN(0, 8),
    .mosi = PIN(0, 9),
    .sck = PIN(1, 29),
    .priority = 0,
    .channel = 0,
    .mode = 0
};

static const struct GpTimerConfig timerConfig = {
    .frequency = 1000000,
    .event = GPTIMER_MATCH0,
    .channel = GPTIMER_CT32B1
};

static const struct UsbDeviceConfig usbConfig = {
    .dm = PIN(PORT_USB, PIN_USB_DM),
    .dp = PIN(PORT_USB, PIN_USB_DP),
    .connect = PIN(PORT_0, 6),
    .vbus = PIN(PORT_0, 3),
    .vid = 0x15A2,
    .pid = 0x0044,
    .channel = 0
};

static const struct WdtConfig wdtConfig = {
    .period = 1000
};
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};

static const struct PllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 4,
    .multiplier = 24
};

static const struct PllConfig usbPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 4,
    .multiplier = 16
};

static const struct GenericClockConfig mainClockConfigExt = {
    .source = CLOCK_EXTERNAL
};

static const struct GenericClockConfig mainClockConfigPll = {
    .source = CLOCK_PLL
};

static const struct GenericClockConfig usbClockConfig = {
    .source = CLOCK_USB_PLL
};

static const struct WdtOscConfig wdtOscConfig = {
    .frequency = WDT_FREQ_2100,
    .divisor = 2
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
void boardSetupClockOutput(uint32_t divisor)
{
  /* Override default config */
  struct ClockOutputConfig config = clockOutputConfig;
  config.divisor = divisor;

  clockEnable(ClockOutput, &config);
  while (!clockReady(ClockOutput));
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupAdc(void)
{
  struct Interface * const interface = init(Adc, &adcConfig);
  assert(interface);
  return(interface);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupAdcOneShot(void)
{
  struct Interface * const interface = init(AdcOneShot, &adcOneShotConfig);
  assert(interface);
  return(interface);
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
  struct Interrupt * const interrupt = init(PinInt, &buttonIntConfig);
  assert(interrupt);
  return(interrupt);
}
/*----------------------------------------------------------------------------*/
struct CapturePackage boardSetupCapture(void)
{
  struct GpTimerCaptureUnit * const timer =
      init(GpTimerCaptureUnit, &captureTimerConfig);
  assert(timer);

  struct Capture * const capture =
      gpTimerCaptureCreate(timer, BOARD_CAPTURE, PIN_RISING, PIN_PULLDOWN);
  assert(capture);

  return (struct CapturePackage){(struct Timer *)timer, capture};
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupCounterTimer(void)
{
  struct Timer * const timer = init(GpTimerCounter, &counterTimerConfig);
  assert(timer);
  return(timer);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupI2C(void)
{
  struct Interface * const interface = init(I2C, &i2cConfig);
  assert(interface);
  return(interface);
}
/*----------------------------------------------------------------------------*/
struct PwmPackage boardSetupPwm(void)
{
  struct GpTimerPwmUnit * const timer = init(GpTimerPwmUnit, &pwmTimerConfig);
  assert(timer);

  struct Pwm * const pwm0 = gpTimerPwmCreate(timer, BOARD_PWM_0);
  assert(pwm0);
  struct Pwm * const pwm1 = gpTimerPwmCreate(timer, BOARD_PWM_1);
  assert(pwm1);

  return (struct PwmPackage){
      (struct Timer *)timer,
      pwm0,
      {pwm0, pwm1}
  };
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerial(void)
{
  struct Interface * const interface = init(Serial, &serialConfig);
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
  clockEnable(UsbPll, &usbPllConfig);
  while (!clockReady(UsbPll));

  /*
   * System PLL and USB PLL should be running to make both clock
   * sources available for switching. After the switch, the USB PLL
   * can be turned off.
   */
  clockEnable(UsbClock, &usbClockConfig);
  while (!clockReady(UsbClock));

  struct Entity * const usb = init(UsbDevice, &usbConfig);
  assert(usb);
  return(usb);
}
/*----------------------------------------------------------------------------*/
struct Watchdog *boardSetupWdt(void)
{
  clockEnable(WdtOsc, &wdtOscConfig);
  while (!clockReady(WdtOsc));

  struct Watchdog * const timer = init(Wdt, &wdtConfig);
  assert(timer);
  return(timer);
}
