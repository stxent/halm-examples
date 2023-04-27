/*
 * lpc11xx_default/shared/board.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/lpc/adc.h>
#include <halm/platform/lpc/adc_oneshot.h>
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/gptimer.h>
#include <halm/platform/lpc/i2c.h>
#include <halm/platform/lpc/pin_int.h>
#include <halm/platform/lpc/serial.h>
#include <halm/platform/lpc/spi.h>
#include <halm/platform/lpc/wdt.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
const PinNumber adcPinArray[] = {
    PIN(1, 10),
    0
};

static const struct AdcConfig adcConfig = {
    .pins = adcPinArray,
    .event = ADC_CT32B0_MAT0,
    .channel = 0
};

static const struct AdcOneShotConfig adcOneShotConfig = {
    .pin = PIN(1, 10),
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

static const struct I2CConfig i2cConfig = {
    .rate = 100000,
    .scl = PIN(0, 4),
    .sda = PIN(0, 5),
    .channel = 0
};

static const struct SerialConfig serialConfig = {
    .rxLength = BOARD_UART_BUFFER,
    .txLength = BOARD_UART_BUFFER,
    .rate = 19200,
    .rx = PIN(1, 6),
    .tx = PIN(1, 7),
    .channel = 0
};

static const struct SpiConfig spiConfig = {
    .rate = 2000000,
    .miso = PIN(0, 8),
    .mosi = PIN(0, 9),
    .sck = PIN(0, 6),
    .channel = 0,
    .mode = 0
};

static const struct GpTimerConfig timerConfig = {
    .frequency = 1000000,
    .event = GPTIMER_MATCH0,
    .channel = GPTIMER_CT32B1
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
    .divisor = 4,
    .multiplier = 16
};

static const struct GenericClockConfig mainClockConfigExt = {
    .source = CLOCK_EXTERNAL
};

static const struct GenericClockConfig mainClockConfigPll = {
    .source = CLOCK_PLL
};

static const struct WdtOscConfig wdtOscConfig = {
    .frequency = WDT_FREQ_1050
};

static const struct GenericClockConfig wdtClockConfig = {
    .source = CLOCK_WDT
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
struct Interface *boardSetupAdc(void)
{
  struct Interface * const interface = init(Adc, &adcConfig);
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
struct Interface *boardSetupI2C(void)
{
  struct Interface * const interface = init(I2C, &i2cConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerial(void)
{
  struct Interface * const interface = init(Serial, &serialConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpi(void)
{
  struct Interface * const interface = init(Spi, &spiConfig);
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
struct Watchdog *boardSetupWdt(void)
{
  clockEnable(WdtOsc, &wdtOscConfig);
  while (!clockReady(WdtOsc));

  clockEnable(WdtClock, &wdtClockConfig);
  while (!clockReady(WdtClock));

  struct Watchdog * const timer = init(Wdt, &wdtConfig);
  assert(timer != NULL);
  return timer;
}
