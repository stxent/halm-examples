/*
 * bl602_default/shared/board.c
 * Copyright (C) 2026 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/core/riscv/machine_timer.h>
#include <halm/platform/bouffalo/clocking.h>
#include <halm/platform/bouffalo/gptimer.h>
#include <halm/platform/bouffalo/i2c.h>
#include <halm/platform/bouffalo/pwm.h>
#include <halm/platform/bouffalo/serial.h>
#include <halm/platform/bouffalo/serial_dma.h>
#include <halm/platform/bouffalo/spi.h>
#include <halm/platform/bouffalo/spi_dma.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 40000000
};

static const struct DividedClockConfig socClockConfig = {
    .divisor = 1
};
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void)
{
  static const struct GenericClockConfig flashClockConfigDefault = {
      .divisor = 1,
      .source = CLOCK_SYSTEM
  };
  static const struct GenericClockConfig mainClockConfigExt = {
      .divisor = 1,
      .source = CLOCK_EXTERNAL
  };

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainClock, &mainClockConfigExt);
  clockEnable(FlashClock, &flashClockConfigDefault);
  clockEnable(SocClock, &socClockConfig);
}
/*----------------------------------------------------------------------------*/
void boardSetupClockPll(void)
{
  static const struct GenericClockConfig flashClockConfigDefault = {
      .divisor = 1,
      .source = CLOCK_SYSTEM
  };
    static const struct GenericClockConfig flashClockConfigPll = {
      .divisor = 2,
      .source = CLOCK_PLL_80MHZ
  };
  static const struct GenericClockConfig mainClockConfigInt = {
      .divisor = 1,
      .source = CLOCK_INTERNAL
  };
  static const struct GenericClockConfig mainClockConfigPll = {
      .divisor = 1,
      .source = CLOCK_PLL_160MHZ
  };

  clockEnable(MainClock, &mainClockConfigInt);
  clockEnable(FlashClock, &flashClockConfigDefault);

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &(struct PllConfig){CLOCK_EXTERNAL});
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &mainClockConfigPll);
  clockEnable(FlashClock, &flashClockConfigPll);
  clockEnable(SocClock, &socClockConfig);
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupI2C(void)
{
  static const struct I2CConfig i2cConfig = {
      .rate = 400000,
      .scl = PIN(0, 4),
      .sda = PIN(0, 3),
      .channel = 0
  };
  /* Use recommended 16 MHz intermediate frequency, I2C rate will be 250 kHz */
  static const struct GenericClockConfig i2cClockConfig = {
      .divisor = 10,
      .source = CLOCK_SYSTEM
  };

  clockEnable(I2CClock, &i2cClockConfig);
  while (!clockReady(UartClock));

  struct Interface * const interface = init(I2C, &i2cConfig);
  assert(interface != nullptr);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct PwmPackage boardSetupPwm(bool)
{
  static const struct PwmUnitConfig pwmTimerConfigs[3] = {
      {
          .frequency = 1000000,
          .resolution = 20000,
          .channel = 4
      }, {
          .frequency = 1000000,
          .resolution = 20000,
          .channel = 3
      }, {
          .frequency = 1000000,
          .resolution = 20000,
          .channel = 1
      }
  };
  static const PinNumber pwmOutputConfigs[3] = {
      BOARD_PWM_0,
      BOARD_PWM_1,
      BOARD_PWM_2
  };
  const bool inversion = false;
  struct PwmPackage package;

  static_assert(ARRAY_SIZE(pwmTimerConfigs) == ARRAY_SIZE(pwmOutputConfigs));
  static_assert(ARRAY_SIZE(pwmTimerConfigs) == ARRAY_SIZE(package.timers));
  static_assert(ARRAY_SIZE(pwmOutputConfigs) == ARRAY_SIZE(package.outputs));

  for (size_t i = 0; i < ARRAY_SIZE(pwmTimerConfigs); ++i)
  {
    package.timers[i] = init(PwmUnit, &pwmTimerConfigs[i]);
    assert(package.timers[i] != nullptr);

    if (i >= 2)
    {
      package.outputs[i] = pwmCreateDoubleEdge(package.timers[i],
          pwmOutputConfigs[i], inversion);
    }
    else
    {
      package.outputs[i] = pwmCreate(package.timers[i],
          pwmOutputConfigs[i], inversion);
    }
    assert(package.outputs[i] != nullptr);
  }

  package.timer = package.timers[0];
  package.output = package.outputs[0];
  return package;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerial(void)
{
  static const struct SerialConfig serialConfig = {
      .rxLength = BOARD_UART_BUFFER,
      .txLength = BOARD_UART_BUFFER,
      .rate = 19200,
      .rx = PIN(0, 7),
      .tx = PIN(0, 16),
      .channel = 0
  };
  static const struct GenericClockConfig uartClockConfig = {
      .divisor = 1,
      .source = CLOCK_SYSTEM
  };

  clockEnable(UartClock, &uartClockConfig);
  while (!clockReady(UartClock));

  struct Interface * const interface = init(Serial, &serialConfig);
  assert(interface != nullptr);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerialDma(void)
{
  static const struct SerialDmaConfig serialDmaConfig = {
      .rxChunk = BOARD_UART_BUFFER / 4,
      .rxLength = BOARD_UART_BUFFER,
      .txLength = BOARD_UART_BUFFER,
      .rate = 19200,
      .rx = PIN(0, 7),
      .tx = PIN(0, 16),
      .channel = 0,
      .dma = {0, 1}
  };
  static const struct GenericClockConfig uartClockConfig = {
      .divisor = 1,
      .source = CLOCK_SYSTEM
  };

  clockEnable(UartClock, &uartClockConfig);
  while (!clockReady(UartClock));

  struct Interface * const interface = init(SerialDma, &serialDmaConfig);
  assert(interface != nullptr);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpi(void)
{
  static const struct SpiConfig spiConfig = {
      .rate = 2000000,
      .miso = PIN(0, 4),
      .mosi = PIN(0, 5),
      .sck = PIN(0, 7),
      .channel = 0,
      .mode = 3
  };
  static const struct DividedClockConfig spiClockConfig = {
      .divisor = 4
  };

  clockEnable(SpiClock, &spiClockConfig);
  while (!clockReady(SpiClock));

  struct Interface * const interface = init(Spi, &spiConfig);
  assert(interface != nullptr);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSpiDma(void)
{
  static const struct SpiDmaConfig spiConfig = {
      .rate = 2000000,
      .miso = PIN(0, 4),
      .mosi = PIN(0, 5),
      .sck = PIN(0, 7),
      .channel = 0,
      .mode = 3,
      .dma = {2, 3}
  };
  static const struct DividedClockConfig spiClockConfig = {
      .divisor = 4
  };

  clockEnable(SpiClock, &spiClockConfig);
  while (!clockReady(SpiClock));

  struct Interface * const interface = init(SpiDma, &spiConfig);
  assert(interface != nullptr);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimer(void)
{
  static const struct GpTimerConfig timerConfig = {
      .frequency = 1000000,
      .channel = TIM2
  };

  struct Timer * const timer = init(GpTimer, &timerConfig);
  assert(timer != nullptr);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Timer64 *boardSetupTimer64(void)
{
  struct Timer64 * const timer = init(MachineTimer64, nullptr);
  assert(timer != nullptr);
  return timer;
}
