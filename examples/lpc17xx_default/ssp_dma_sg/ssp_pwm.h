/*
 * lpc17xx_default/ssp_dma_sg/ssp_pwm.h
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef LPC17XX_DEFAULT_SSP_DMA_SG_SSP_PWM_H_
#define LPC17XX_DEFAULT_SSP_DMA_SG_SSP_PWM_H_
/*----------------------------------------------------------------------------*/
#include <halm/dma.h>
#include <halm/platform/nxp/ssp_base.h>
#include <halm/timer.h>
/*----------------------------------------------------------------------------*/
extern const struct EntityClass * const SspPwm;

struct SspPwmConfig
{
  /** Mandatory: lighting style. */
  uint32_t (*style)(size_t, size_t);

  /** Optional: Chip Select output. */
  PinNumber cs;
  /** Mandatory: serial data output. */
  PinNumber mosi;
  /** Mandatory: serial clock output. */
  PinNumber sck;

  /** Mandatory: peripheral identifier. */
  uint8_t channel;
  /** Mandatory: outgoing data channel. */
  uint8_t dma;
  /** Mandatory: timer peripheral identifier. */
  uint8_t timer;

  /** Mandatory: strip length. */
  size_t length;
  /** Mandatory: PWM frequency. */
  uint32_t frequency;
  /** Mandatory: PWM resolution. */
  uint32_t resolution;
};

struct SspPwm
{
  struct SspBase base;

  struct Dma *dma;
  struct Timer *timer;

  uint16_t *buffer;
};
/*----------------------------------------------------------------------------*/
#endif /* LPC17XX_DEFAULT_SSP_DMA_SG_SSP_PWM_H_ */
