/*
 * lpc17xx_default/ssp_dma_sg/ssp_pwm.c
 * Copyright (C) 2014 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "ssp_pwm.h"
#include <halm/platform/nxp/gpdma_circular.h>
#include <halm/platform/nxp/gptimer.h>
#include <halm/platform/nxp/ssp_defs.h>
#include <assert.h>
#include <stdlib.h>
/*----------------------------------------------------------------------------*/
static void fillBuffer(uint16_t *, uint32_t (*)(size_t, size_t), size_t,
    size_t, int);
static void prepareBuffers(struct SspPwm *, uint16_t *,
    uint32_t (*)(size_t, size_t), size_t, size_t);
static bool setupDataChannel(struct SspPwm *, uint8_t, uint8_t, size_t);
static void setupInterface(struct SspPwm *);
/*----------------------------------------------------------------------------*/
static enum Result objectInit(void *, const void *);
static void objectDeinit(void *);
/*----------------------------------------------------------------------------*/
const struct EntityClass * const SspPwm = &(const struct EntityClass){
    .size = sizeof(struct SspPwm),
    .init = objectInit,
    .deinit = objectDeinit
};
/*----------------------------------------------------------------------------*/
static void fillBuffer(uint16_t *buffer, uint32_t (*style)(size_t, size_t),
    size_t resolution, size_t length, int iteration)
{
  const int position = iteration % ((length - 1) * 2) - (length - 1);

  for (size_t i = 0; i < resolution; ++i)
    buffer[i] = style(length, i) >> abs(position);
}
/*----------------------------------------------------------------------------*/
static void prepareBuffers(struct SspPwm *controller, uint16_t *buffer,
    uint32_t (*style)(size_t, size_t), size_t resolution, size_t length)
{
  for (size_t i = 0; i < length; ++i)
    fillBuffer(buffer + i * resolution, style, resolution, length, i);

  LPC_SSP_Type * const reg = controller->base.reg;
  void * const target = (void *)&reg->DR;

  for (size_t i = 0; i < length - 1; ++i)
    dmaAppend(controller->dma, target, buffer + i * resolution, resolution);
  for (size_t i = length - 1; i > 0; --i)
    dmaAppend(controller->dma, target, buffer + i * resolution, resolution);
}
/*----------------------------------------------------------------------------*/
static bool setupDataChannel(struct SspPwm *controller, uint8_t channel,
    uint8_t timer, size_t length)
{
  static const struct GpDmaSettings dmaSettings = {
      .source = {
          .burst = DMA_BURST_4,
          .width = DMA_WIDTH_HALFWORD,
          .increment = true
      },
      .destination = {
          .burst = DMA_BURST_1,
          .width = DMA_WIDTH_HALFWORD,
          .increment = false
      }
  };
  const struct GpDmaCircularConfig dmaConfig = {
      .number = (length - 1) * 2,
      .event = GPDMA_MAT0_0 + timer * 2,
      .type = GPDMA_TYPE_M2P,
      .channel = channel,
      .silent = false
  };

  controller->dma = init(GpDmaCircular, &dmaConfig);
  if (!controller->dma)
    return false;
  dmaConfigure(controller->dma, &dmaSettings);

  return true;
}
/*----------------------------------------------------------------------------*/
static void setupInterface(struct SspPwm *controller)
{
  LPC_SSP_Type * const reg = controller->base.reg;

  /* Set frame size */
  reg->CR0 = CR0_DSS(16);

  sspSetRate((struct SspBase *)controller, 1000000);

  /* Enable peripheral */
  reg->CR1 = CR1_SSE;

  /* Clear timeout interrupt flags */
  reg->ICR = ICR_RORIC | ICR_RTIC;

  /* Clear DMA requests */
  reg->DMACR &= ~(DMACR_RXDMAE | DMACR_TXDMAE);
  reg->DMACR |= DMACR_RXDMAE | DMACR_TXDMAE;
}
/*----------------------------------------------------------------------------*/
static enum Result objectInit(void *object, const void *configPtr)
{
  const struct SspPwmConfig * const config = configPtr;
  const struct GpTimerConfig timerConfig = {
      .frequency = 2 * config->frequency,
      .event = GPTIMER_MATCH0,
      .priority = 0,
      .channel = config->timer
  };
  const struct SspBaseConfig parentConfig = {
      .cs = 0,
      .miso = 0,
      .mosi = config->mosi,
      .sck = config->sck,
      .channel = config->channel
  };
  struct SspPwm * const controller = object;
  enum Result res;

  assert(config->resolution > 0);
  assert(config->length > 0 && config->length <= 16);

  /* Call base class constructor */
  if ((res = SspBase->init(object, &parentConfig)) != E_OK)
    return res;

  if (!(controller->timer = init(GpTimer, &timerConfig)))
    return E_ERROR;
  timerSetOverflow(controller->timer, 2);

  setupInterface(controller);
  if (config->cs)
    pinOutput(pinInit(config->cs), false);

  if (!setupDataChannel(controller, config->dma, config->timer, config->length))
    return E_ERROR;

  controller->buffer =
      malloc(config->length * config->resolution * sizeof(uint16_t));
  if (!controller->buffer)
    return E_MEMORY;

  prepareBuffers(controller, controller->buffer, config->style,
      config->resolution, config->length);

  if ((res = dmaEnable(controller->dma)) != E_OK)
    return res;
  timerEnable(controller->timer);

  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void objectDeinit(void *object)
{
  struct SspPwm * const controller = object;

  dmaDisable(controller->dma);
  free(controller->buffer);

  deinit(controller->dma);
  deinit(controller->timer);
  SspBase->deinit(controller);
}
