/*
 * stm32f0xx_default/shared/board.c
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/stm32/clocking.h>
#include <halm/platform/stm32/gptimer.h>
#include <halm/platform/stm32/serial.h>
#include <halm/platform/stm32/spi.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
static const struct SerialConfig serialConfig = {
    .rxLength = BOARD_UART_BUFFER,
    .txLength = BOARD_UART_BUFFER,
    .rate = 19200,
    .rx = PIN(PORT_A, 3),
    .tx = PIN(PORT_A, 2),
    .channel = USART2
};

static const struct SpiConfig spiConfig = {
    .rate = 2000000,
    .miso = PIN(PORT_A, 6),
    .mosi = PIN(PORT_A, 7),
    .sck = PIN(PORT_A, 5),
    .channel = SPI1,
    .mode = 0,
    .rxDma = DMA1_STREAM2,
    .txDma = DMA1_STREAM3
};

static const struct GpTimerConfig timerConfig = {
    .frequency = 10000,
    .channel = TIM14
};
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 8000000
};

static const struct SystemPllConfig sysPllConfig = {
    .source = CLOCK_EXTERNAL,
    .divisor = 1,
    .multiplier = 6
};

static const struct BusClockConfig ahbClockConfig = {
    .divisor = 1
};

static const struct BusClockConfig apbClockConfig = {
    .divisor = 1
};

static const struct SystemClockConfig systemClockConfigExt = {
    .source = CLOCK_EXTERNAL
};

static const struct SystemClockConfig systemClockConfigPll = {
    .source = CLOCK_PLL
};
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(ApbClock, &apbClockConfig);
  clockEnable(SystemClock, &systemClockConfigExt);

  clockEnable(MainClock, &ahbClockConfig);
}
/*----------------------------------------------------------------------------*/
void boardSetupClockPll(void)
{
  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(ApbClock, &apbClockConfig);
  clockEnable(SystemClock, &systemClockConfigPll);

  clockEnable(MainClock, &ahbClockConfig);
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
