/*
 * lpc13uxx_default/shared/board.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/lpc/adc.h>
#include <halm/platform/lpc/adc_oneshot.h>
#include <halm/platform/lpc/bod.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/eeprom.h>
#include <halm/platform/lpc/flash.h>
#include <halm/platform/lpc/gptimer.h>
#include <halm/platform/lpc/gptimer_capture.h>
#include <halm/platform/lpc/gptimer_counter.h>
#include <halm/platform/lpc/gptimer_pwm.h>
#include <halm/platform/lpc/i2c.h>
#include <halm/platform/lpc/pin_int.h>
#include <halm/platform/lpc/serial.h>
#include <halm/platform/lpc/spi.h>
#include <halm/platform/lpc/usb_device.h>
#include <halm/platform/lpc/wwdt.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
static const PinNumber adcPinArray[] = {
    PIN(0, 11),
    PIN(0, 23),
    0
};

static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};
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
void boardSetupClockExt(void)
{
  static const struct GenericClockConfig mainClockConfigExt = {
      .source = CLOCK_EXTERNAL
  };

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainClock, &mainClockConfigExt);
}
/*----------------------------------------------------------------------------*/
const struct ClockClass *boardSetupClockOutput(uint32_t divisor)
{
  const struct ClockOutputConfig clockOutputConfig = {
      .divisor = divisor,
      .pin = PIN(0, 1),
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
      .multiplier = 24,
      .source = CLOCK_EXTERNAL
  };
  static const struct GenericClockConfig mainClockConfigPll = {
      .source = CLOCK_PLL
  };

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &mainClockConfigPll);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupAdc(void)
{
  static const struct AdcConfig adcConfig = {
      .pins = adcPinArray,
      .event = ADC_CT32B0_MAT0,
      .channel = 0
  };

  struct Interface * const interface = init(Adc, &adcConfig);
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
struct Timer *boardSetupAdcTimer(void)
{
  static const struct GpTimerConfig adcTimerConfig = {
      .frequency = 1000000,
      .event = GPTIMER_MATCH0,
      .channel = GPTIMER_CT32B0
  };

  struct Timer * const timer = init(GpTimer, &adcTimerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Interrupt *boardSetupBod(void)
{
  static const struct BodConfig bodConfig = {
      .eventLevel = BOD_EVENT_2V80,
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
      .event = INPUT_FALLING,
      .pull = PIN_PULLUP
  };

  struct Interrupt * const interrupt = init(PinInt, &buttonIntConfig);
  assert(interrupt != NULL);
  return interrupt;
}
/*----------------------------------------------------------------------------*/
struct CapturePackage boardSetupCapture(void)
{
  static const struct GpTimerCaptureUnitConfig captureTimerConfig = {
      .frequency = 1000000,
      .channel = GPTIMER_CT32B0
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
      .channel = GPTIMER_CT32B0
  };

  struct Timer * const timer = init(GpTimerCounter, &counterTimerConfig);
  assert(timer != NULL);
  return timer;
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
  struct Interface * const interface = init(Flash, NULL);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupI2C(void)
{
  static const struct I2CConfig i2cConfig = {
      .rate = 100000,
      .scl = PIN(0, 4),
      .sda = PIN(0, 5),
      .channel = 0
  };

  struct Interface * const interface = init(I2C, &i2cConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct PwmPackage boardSetupPwm(bool centered __attribute__((unused)))
{
  static const struct GpTimerPwmUnitConfig pwmTimerConfig = {
      .frequency = 1000000,
      .resolution = 20000,
      .channel = GPTIMER_CT16B1
  };

  struct GpTimerPwmUnit * const timer = init(GpTimerPwmUnit, &pwmTimerConfig);
  assert(timer != NULL);

  struct Pwm * const pwm0 = gpTimerPwmCreate(timer, BOARD_PWM_0);
  assert(pwm0 != NULL);
  struct Pwm * const pwm1 = gpTimerPwmCreate(timer, BOARD_PWM_1);
  assert(pwm1 != NULL);

  return (struct PwmPackage){
      (struct Timer *)timer,
      pwm0,
      {pwm0, pwm1, NULL}
  };
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerial(void)
{
  static const struct SerialConfig serialConfig = {
      .rxLength = BOARD_UART_BUFFER,
      .txLength = BOARD_UART_BUFFER,
      .rate = 19200,
      .rx = PIN(0, 18),
      .tx = PIN(0, 19),
      .channel = 0
  };

  struct Interface * const interface = init(Serial, &serialConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpi(void)
{
  static const struct SpiConfig spiConfig = {
      .rate = 2000000,
      .miso = PIN(0, 8),
      .mosi = PIN(0, 9),
      .sck = PIN(1, 29),
      .channel = 0,
      .mode = 0
  };

  struct Interface * const interface = init(Spi, &spiConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimer(void)
{
  static const struct GpTimerConfig timerConfig = {
      .frequency = 1000000,
      .event = GPTIMER_MATCH0,
      .channel = GPTIMER_CT32B1
  };

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Entity *boardSetupUsb(void)
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
      .dm = PIN(PORT_USB, PIN_USB_DM),
      .dp = PIN(PORT_USB, PIN_USB_DP),
      .connect = PIN(PORT_0, 6),
      .vbus = PIN(PORT_0, 3),
      .vid = 0x15A2,
      .pid = 0x0044,
      .channel = 0
  };

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
  assert(usb != NULL);
  return usb;
}
/*----------------------------------------------------------------------------*/
struct Watchdog *boardSetupWdt(bool disarmed)
{
  /* Clocks */
  static const struct WdtOscConfig wdtOscConfig = {
      .frequency = WDT_FREQ_2100,
      .divisor = 2
  };

  /* Objects */
  const struct WwdtConfig wwdtConfig = {
      .period = 5000,
      .window = 0,
      .disarmed = disarmed
  };

  clockEnable(WdtOsc, &wdtOscConfig);
  while (!clockReady(WdtOsc));

  struct Watchdog * const timer = init(Wwdt, &wwdtConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Watchdog *boardSetupWwdt(void)
{
  /* Clocks */
  static const struct WdtOscConfig wdtOscConfig = {
      .frequency = WDT_FREQ_2100,
      .divisor = 2
  };

  /* Objects */
  const struct WwdtConfig wwdtConfig = {
      .period = 5000,
      .window = 1000,
      .disarmed = false
  };

  clockEnable(WdtOsc, &wdtOscConfig);
  while (!clockReady(WdtOsc));

  struct Watchdog * const timer = init(Wwdt, &wwdtConfig);
  assert(timer != NULL);
  return timer;
}
