/*
 * lpc43xx_default/shared/sct_adc.h
 * Copyright (C) 2024 xent
 * Project is distributed under the terms of the MIT License
 */

#ifndef LPC43XX_DEFAULT_SHARED_SCT_ADC_H_
#define LPC43XX_DEFAULT_SHARED_SCT_ADC_H_
/*----------------------------------------------------------------------------*/
#include <halm/platform/lpc/sct_base.h>
/*----------------------------------------------------------------------------*/
extern const struct TimerClass * const SctAdc;

enum [[gnu::packed]] SctAdcDmaOutput
{
  SCTADC_DMA_0,
  SCTADC_DMA_1,
  SCTADC_DMA_OUTPUT_2,
  SCTADC_DMA_OUTPUT_3,

  SCTADC_DMA_END
};

enum [[gnu::packed]] SctAdcOutput
{
  SCTADC_ADC_OUTPUT_8,
  SCTADC_ADC_OUTPUT_15,

  SCTADC_ADC_END
};

struct SctAdcConfig
{
  /** Mandatory: cycle time in timer ticks. */
  uint32_t cycle;
  /** Mandatory: conversion time in timer ticks. */
  uint32_t delay;
  /**
   * Optional: desired timer tick rate. An actual peripheral frequency is used
   * when this parameter is set to zero.
   */
  uint32_t frequency;
  /** Mandatory: output event for ADC. */
  enum SctAdcOutput adc;
  /** Mandatory: output event for DMA. */
  enum SctAdcDmaOutput dma;
  /** Optional: timer part. */
  enum SctPart part;
  /** Mandatory: peripheral identifier. */
  uint8_t channel;
};

struct SctAdc
{
  struct SctBase base;

  /* Desired timer frequency */
  uint32_t frequency;

  /* Output event for ADC */
  enum SctAdcOutput adc;
  /* Output event for DMA */
  enum SctAdcDmaOutput dma;

  /* ADC event channel */
  int8_t adcOutput;
  /* DMA event channel */
  int8_t dmaOutput;

  /* Match channel used for ADC conversion event */
  uint8_t conversion;
    /* Match channel used for DMA event */
  uint8_t memory;
  /* Match channel used for counter reset */
  uint8_t reset;
};
/*----------------------------------------------------------------------------*/
#endif /* LPC43XX_DEFAULT_SHARED_SCT_ADC_H_ */
