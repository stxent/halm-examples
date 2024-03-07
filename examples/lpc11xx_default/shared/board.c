/*
 * lpc11xx_default/shared/board.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/lpc/adc.h>
#include <halm/platform/lpc/adc_oneshot.h>
#include <halm/platform/lpc/bod.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/flash.h>
#include <halm/platform/lpc/gptimer.h>
#include <halm/platform/lpc/i2c.h>
#include <halm/platform/lpc/pin_int.h>
#include <halm/platform/lpc/serial.h>
#include <halm/platform/lpc/spi.h>
#include <halm/platform/lpc/wwdt.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
[[gnu::alias("boardSetupTimer32B0")]] struct Timer *boardSetupAdcTimer(void);
[[gnu::alias("boardSetupTimer32B1")]] struct Timer *boardSetupTimer(void);
/*----------------------------------------------------------------------------*/
static const PinNumber adcPinArray[] = {
    PIN(1, 10),
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
void boardSetupClockPll(void)
{
  static const struct PllConfig sysPllConfig = {
      .divisor = 4,
      .multiplier = 16,
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
      .event = BOARD_BUTTON_INV ? INPUT_FALLING : INPUT_RISING,
      .pull = BOARD_BUTTON_INV ? PIN_PULLUP : PIN_PULLDOWN
  };

  struct Interrupt * const interrupt = init(PinInt, &buttonIntConfig);
  assert(interrupt != NULL);
  return interrupt;
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
struct Interface *boardSetupSerial(void)
{
  static const struct SerialConfig serialConfig = {
      .rxLength = BOARD_UART_BUFFER,
      .txLength = BOARD_UART_BUFFER,
      .rate = 19200,
      .rx = PIN(1, 6),
      .tx = PIN(1, 7),
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
      .sck = PIN(0, 6),
      .channel = 0,
      .mode = 0
  };

  struct Interface * const interface = init(Spi, &spiConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimer16B0(void)
{
  static const struct GpTimerConfig timerConfig = {
      .frequency = 10000,
      .event = GPTIMER_MATCH0,
      .channel = GPTIMER_CT16B0
  };

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimer16B1(void)
{
  static const struct GpTimerConfig timerConfig = {
      .frequency = 10000,
      .event = GPTIMER_MATCH0,
      .channel = GPTIMER_CT16B1
  };

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimer32B0(void)
{
  static const struct GpTimerConfig adcTimerConfig = {
      .frequency = 1000000,
      .event = GPTIMER_MATCH0, /* Used as an ADC trigger */
      .channel = GPTIMER_CT32B0
  };

  struct Timer * const timer = init(GpTimer, &adcTimerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimer32B1(void)
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
struct Watchdog *boardSetupWdt([[maybe_unused]] bool disarmed)
{
  /* Clocks */
  static const struct WdtOscConfig wdtOscConfig = {
      .frequency = WDT_FREQ_1050
  };
  static const struct GenericClockConfig wdtClockConfig = {
      .source = CLOCK_WDT
  };

  /* Objects */
  const struct WwdtConfig wwdtConfig = {
      .period = 5000,
      .window = 0,
      .disarmed = disarmed
  };

  clockEnable(WdtOsc, &wdtOscConfig);
  while (!clockReady(WdtOsc));

  clockEnable(WdtClock, &wdtClockConfig);
  while (!clockReady(WdtClock));

  struct Watchdog * const timer = init(Wwdt, &wwdtConfig);
  assert(timer != NULL);
  return timer;
}
