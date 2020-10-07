/*
 * lpc17xx_default/ssp_dma_sg/main.c
 * Copyright (C) 2016 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "ssp_pwm.h"
#include <xcore/bits.h>
/*----------------------------------------------------------------------------*/
#define SSP_CHANNEL 0
/*----------------------------------------------------------------------------*/
uint32_t linearFunction(size_t length, size_t index)
{
  const uint32_t rawMask = MASK(index * 2 + 1);
  const uint32_t shiftedMask = (rawMask << (length - 1)) >> index;
  return ~shiftedMask;
}
/*----------------------------------------------------------------------------*/
uint32_t unitStepFunction(size_t length, size_t index __attribute__((unused)))
{
  return (1 << (length - 1));
}
/*----------------------------------------------------------------------------*/
static const struct SspPwmConfig config[] = {
    {
        .style      = unitStepFunction,

        .cs         = PIN(0, 16),
        .sck        = PIN(0, 15),
        .mosi       = PIN(0, 18),

        .channel    = 0,
        .dma        = 0,
        .timer      = 0,

        .length     = 10,
        .frequency  = 500,
        .resolution = 10
    },
    {
        .style      = linearFunction,

        .cs         = PIN(0, 6),
        .sck        = PIN(0, 7),
        .mosi       = PIN(0, 9),

        .channel    = 1,
        .dma        = 1,
        .timer      = 1,

        .length     = 10,
        .frequency  = 500,
        .resolution = 10
    }
};
/*----------------------------------------------------------------------------*/
int main(void)
{
  init(SspPwm, &config[SSP_CHANNEL]);

  while (1);
  return 0;
}
