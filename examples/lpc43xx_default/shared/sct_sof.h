/*
 * lpc43xx_default/shared/sct_sof.h
 * Copyright (C) 2025 xent
 * Project is distributed under the terms of the MIT License
 */

#ifndef LPC43XX_DEFAULT_SHARED_SCT_SOF_H_
#define LPC43XX_DEFAULT_SHARED_SCT_SOF_H_
/*----------------------------------------------------------------------------*/
#include <halm/platform/lpc/sct_base.h>
/*----------------------------------------------------------------------------*/
extern const struct TimerClass * const SctSof;

enum [[gnu::packed]] SctSofInput
{
  SCTSOF_I2S0_RX_MWS_6,
  SCTSOF_I2S0_TX_MWS_6,
  SCTSOF_I2S1_RX_MWS_3,
  SCTSOF_I2S1_TX_MWS_3,
  SCTSOF_I2S1_RX_MWS_4,
  SCTSOF_I2S1_TX_MWS_4,
  SCTSOF_USB0_SOF_7,
  SCTSOF_USB1_SOF_7,

  SCTSOF_END
};

struct SctSofConfig
{
  /**
   * Optional: desired timer tick rate. An actual peripheral frequency is used
   * when this parameter is set to zero.
   */
  uint32_t frequency;
  /** Mandatory: input channel for I2S MWS. */
  enum SctSofInput i2s;
  /** Mandatory: input channel for USB SOF. */
  enum SctSofInput usb;
  /** Optional: timer part. */
  enum SctPart part;
  /** Optional: timer inerrupt priority. */
  IrqPriority priority;
  /** Mandatory: peripheral identifier. */
  uint8_t channel;
};

struct SctSof
{
  struct SctBase base;

  /* Desired timer frequency */
  uint32_t frequency;
  /* Sample rate in Hz */
  uint32_t rate;
  /* MWS duration in timer ticks */
  uint16_t mwsDuration;
  /* Previous timer value for MWS event */
  uint16_t mwsPrevious;
  /* SOF duration in timer ticks */
  uint16_t sofDuration;
  /* Previous timer value for SOF event */
  uint16_t sofPrevious;

  /* I2S input channel */
  enum SctInput i2sInput;
  /* USB input channel */
  enum SctInput usbInput;

  /* Capture channel used for I2S MWS */
  uint8_t i2sEvent;
  /* Capture channel used for USB SOF */
  uint8_t usbEvent;

  /* Previous MWS value obtained */
  bool mwsValid;
  /* Previous SOF value obtained */
  bool sofValid;
};
/*----------------------------------------------------------------------------*/
BEGIN_DECLS

uint32_t sctSofGetRatio(const struct SctSof *);
void sctSofSetSampleRate(struct SctSof *, uint32_t);

END_DECLS
/*----------------------------------------------------------------------------*/
#endif /* LPC43XX_DEFAULT_SHARED_SCT_SOF_H_ */
