/*
 * lpc43xx_default/shared/sct_adc.c
 * Copyright (C) 2024 xent
 * Project is distributed under the terms of the MIT License
 */

#include "sct_adc.h"
#include <halm/platform/lpc/sct_defs.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
static enum Result tmrInit(void *, const void *);
static void tmrDeinit(void *);
static void tmrEnable(void *);
static void tmrDisable(void *);
static uint32_t tmrGetFrequency(const void *);
static void tmrSetFrequency(void *, uint32_t);
static uint32_t tmrGetOverflow(const void *);
static void tmrSetOverflow(void *, uint32_t);
static uint32_t tmrGetValue(const void *);
static void tmrSetValue(void *, uint32_t);
/*----------------------------------------------------------------------------*/
const struct TimerClass * const SctAdc = &(const struct TimerClass){
    .size = sizeof(struct SctAdc),
    .init = tmrInit,
    .deinit = tmrDeinit,

    .enable = tmrEnable,
    .disable = tmrDisable,
    .setAutostop = NULL,
    .setCallback = NULL,
    .getFrequency = tmrGetFrequency,
    .setFrequency = tmrSetFrequency,
    .getOverflow = tmrGetOverflow,
    .setOverflow = tmrSetOverflow,
    .getValue = tmrGetValue,
    .setValue = tmrSetValue
};
/*----------------------------------------------------------------------------*/
static enum Result tmrInit(void *object, const void *configBase)
{
  const struct SctAdcConfig * const config = configBase;
  assert(config != NULL);
  assert(config->adc < SCTADC_ADC_END);
  assert(config->dma < SCTADC_DMA_END);
  assert(config->part != SCT_UNIFIED);

  const struct SctBaseConfig baseConfig = {
      .channel = config->channel,
      .part = config->part
  };
  struct SctAdc * const timer = object;
  enum Result res;

  /* Call base class constructor */
  if ((res = SctBase->init(timer, &baseConfig)) != E_OK)
    return res;

  LPC_SCT_Type * const reg = timer->base.reg;
  const unsigned int part = timer->base.part == SCT_HIGH;

  if (!sctAllocateEvent(&timer->base, &timer->conversion))
    return E_BUSY;
  if (!sctAllocateEvent(&timer->base, &timer->memory))
    return E_BUSY;
  if (!sctAllocateEvent(&timer->base, &timer->reset))
    return E_BUSY;
  timer->base.mask = (1 << timer->conversion)
      | (1 << timer->memory)
      | (1 << timer->reset);

  /* Disable the timer before any configuration is done */
  reg->CTRL_PART[part] = CTRL_HALT;

  /* Set desired timer frequency */
  timer->frequency = config->frequency;
  sctSetFrequency(&timer->base, timer->frequency);

  /* Disable match value reload and set current match register value */
  reg->CONFIG |= CONFIG_NORELOAD(part);

  /* Configure conversion event */
  reg->MATCH_PART[timer->conversion][part] = 1;
  reg->EV[timer->conversion].CTRL =
      (timer->base.part == SCT_HIGH ? EVCTRL_HEVENT : 0)
      | EVCTRL_MATCHSEL(timer->conversion)
      | EVCTRL_COMBMODE(COMBMODE_MATCH)
      | EVCTRL_DIRECTION(DIRECTION_INDEPENDENT);
  reg->EV[timer->conversion].STATE = 0x00000001;
  reg->REGMODE_PART[part] &= ~(1 << timer->conversion);

  timer->adc = config->adc;
  switch (timer->adc)
  {
    case SCTADC_ADC_OUTPUT_8:
      timer->adcOutput = 8;
      break;
    case SCTADC_ADC_OUTPUT_15:
      timer->adcOutput = 15;
      break;
    default:
      break;
  }
  if (timer->adcOutput != -1)
  {
    reg->OUTPUTDIRCTRL &= ~OUTPUTDIRCTRL_SETCLR_MASK(timer->adcOutput);
    reg->RES &= ~RES_OUTPUT_MASK(timer->adcOutput);
    reg->OUT[timer->adcOutput].CLR = 1 << timer->memory;
    reg->OUT[timer->adcOutput].SET = 1 << timer->conversion;
  }

  /* Configure memory transfer event */
  reg->MATCH_PART[timer->memory][part] = 1 + config->delay;
  reg->EV[timer->memory].CTRL =
      (timer->base.part == SCT_HIGH ? EVCTRL_HEVENT : 0)
      | EVCTRL_MATCHSEL(timer->memory)
      | EVCTRL_COMBMODE(COMBMODE_MATCH)
      | EVCTRL_DIRECTION(DIRECTION_INDEPENDENT);
  reg->EV[timer->memory].STATE = 0x00000001;
  reg->REGMODE_PART[part] &= ~(1 << timer->memory);

  timer->dma = config->dma;
  switch (timer->dma)
  {
    case SCTADC_DMA_0:
      timer->dmaOutput = -1;
      reg->DMAREQ0 |= 1 << timer->memory;
      break;
    case SCTADC_DMA_1:
      timer->dmaOutput = -1;
      reg->DMAREQ1 |= 1 << timer->memory;
      break;
    case SCTADC_DMA_OUTPUT_2:
      timer->dmaOutput = 2;
      break;
    case SCTADC_DMA_OUTPUT_3:
      timer->dmaOutput = 3;
      break;
    default:
      break;
  }
  if (timer->dmaOutput != -1)
  {
    reg->OUTPUTDIRCTRL &= ~OUTPUTDIRCTRL_SETCLR_MASK(timer->dmaOutput);
    reg->RES &= ~RES_OUTPUT_MASK(timer->dmaOutput);
    reg->OUT[timer->dmaOutput].CLR = 1 << timer->reset;
    reg->OUT[timer->dmaOutput].SET = 1 << timer->memory;
  }

  /* Configure reset event */
  reg->MATCH_PART[timer->reset][part] = config->cycle;
  reg->EV[timer->reset].CTRL =
      (timer->base.part == SCT_HIGH ? EVCTRL_HEVENT : 0)
      | EVCTRL_MATCHSEL(timer->reset)
      | EVCTRL_COMBMODE(COMBMODE_MATCH)
      | EVCTRL_DIRECTION(DIRECTION_INDEPENDENT);
  reg->EV[timer->reset].STATE = 0x00000001;
  reg->REGMODE_PART[part] &= ~(1 << timer->reset);

  /* Reset current state */
  reg->STATE_PART[part] = 0;
  /* Enable timer clearing on allocated event */
  reg->LIMIT_PART[part] = 1 << timer->reset;

  /* By default the timer is disabled */
  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void tmrDeinit(void *object)
{
  struct SctAdc * const timer = object;
  LPC_SCT_Type * const reg = timer->base.reg;
  const unsigned int part = timer->base.part == SCT_HIGH;

  /* Halt the timer */
  reg->CTRL_PART[part] = CTRL_HALT;
  reg->EVEN &= ~timer->base.mask;
  reg->LIMIT_PART[part] = 0;

  /* Disable ADC and DMA events */
  switch (timer->dma)
  {
    case SCTADC_DMA_0:
      reg->DMAREQ0 &= ~(1 << timer->memory);
      break;
    case SCTADC_DMA_1:
      reg->DMAREQ1 &= ~(1 << timer->memory);
      break;
    default:
      break;
  }
  if (timer->dmaOutput != -1)
  {
    reg->OUT[timer->dmaOutput].CLR = 0;
    reg->OUT[timer->dmaOutput].SET = 0;
  }
  if (timer->adcOutput != -1)
  {
    reg->OUT[timer->adcOutput].CLR = 0;
    reg->OUT[timer->adcOutput].SET = 0;
  }

  /* Disable allocated SCT events */
  reg->EV[timer->reset].CTRL = 0;
  reg->EV[timer->reset].STATE = 0;
  sctReleaseEvent(&timer->base, timer->reset);
  reg->EV[timer->memory].CTRL = 0;
  reg->EV[timer->memory].STATE = 0;
  sctReleaseEvent(&timer->base, timer->memory);
  reg->EV[timer->conversion].CTRL = 0;
  reg->EV[timer->conversion].STATE = 0;
  sctReleaseEvent(&timer->base, timer->conversion);

  /* Reset to default state */
  reg->CONFIG &= ~CONFIG_NORELOAD(part);

  SctBase->deinit(timer);
}
/*----------------------------------------------------------------------------*/
static void tmrEnable(void *object)
{
  struct SctAdc * const timer = object;
  LPC_SCT_Type * const reg = timer->base.reg;
  const unsigned int part = timer->base.part == SCT_HIGH;

  reg->EVFLAG = timer->base.mask;
  reg->CTRL_PART[part] &= ~CTRL_HALT;
}
/*----------------------------------------------------------------------------*/
static void tmrDisable(void *object)
{
  struct SctAdc * const timer = object;
  LPC_SCT_Type * const reg = timer->base.reg;
  const unsigned int part = timer->base.part == SCT_HIGH;

  reg->CTRL_PART[part] |= CTRL_HALT;
}
/*----------------------------------------------------------------------------*/
static uint32_t tmrGetFrequency(const void *object)
{
  const struct SctAdc * const timer = object;
  const LPC_SCT_Type * const reg = timer->base.reg;
  const unsigned int part = timer->base.part == SCT_HIGH;
  const uint32_t baseClock = sctGetClock(&timer->base);
  const uint32_t prescaler = CTRL_PRE_VALUE(reg->CTRL_PART[part]) + 1;

  return baseClock / prescaler;
}
/*----------------------------------------------------------------------------*/
static void tmrSetFrequency(void *object, uint32_t frequency)
{
  struct SctAdc * const timer = object;

  timer->frequency = frequency;
  sctSetFrequency(&timer->base, timer->frequency);
}
/*----------------------------------------------------------------------------*/
static uint32_t tmrGetOverflow(const void *object)
{
  const struct SctAdc * const timer = object;
  const LPC_SCT_Type * const reg = timer->base.reg;
  const unsigned int part = timer->base.part == SCT_HIGH;

  return reg->MATCH_PART[timer->reset][part] + 1;
}
/*----------------------------------------------------------------------------*/
static void tmrSetOverflow(void *object, uint32_t overflow)
{
  struct SctAdc * const timer = object;
  LPC_SCT_Type * const reg = timer->base.reg;
  const unsigned int part = timer->base.part == SCT_HIGH;

  assert(overflow < (1 << 16));

  reg->CTRL_PART[part] |= CTRL_STOP;
  reg->MATCH_PART[timer->reset][part] = overflow - 1;
  reg->CTRL_PART[part] &= ~CTRL_STOP;
}
/*----------------------------------------------------------------------------*/
static uint32_t tmrGetValue(const void *object)
{
  const struct SctAdc * const timer = object;
  const LPC_SCT_Type * const reg = timer->base.reg;
  const unsigned int part = timer->base.part == SCT_HIGH;

  return reg->COUNT_PART[part];
}
/*----------------------------------------------------------------------------*/
static void tmrSetValue(void *object, uint32_t value)
{
  struct SctAdc * const timer = object;
  LPC_SCT_Type * const reg = timer->base.reg;
  const unsigned int part = timer->base.part == SCT_HIGH;

  assert(value <= reg->MATCH_PART[timer->reset][part]);

  reg->CTRL_PART[part] |= CTRL_STOP;
  reg->COUNT_PART[part] = value;
  reg->CTRL_PART[part] &= ~CTRL_STOP;
}
