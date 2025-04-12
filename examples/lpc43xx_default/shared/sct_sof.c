/*
 * lpc43xx_default/shared/sct_sof.c
 * Copyright (C) 2025 xent
 * Project is distributed under the terms of the MIT License
 */

#include "sct_sof.h"
#include <halm/platform/lpc/lpc43xx/gima_defs.h>
#include <halm/platform/lpc/sct_defs.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
static enum SctInput inputToInputChannel(enum SctSofInput);
static void interruptHandler(void *);
static void routeGimaInput(enum SctSofInput);
/*----------------------------------------------------------------------------*/
static enum Result tmrInit(void *, const void *);
static void tmrDeinit(void *);
static void tmrEnable(void *);
static void tmrDisable(void *);
static uint32_t tmrGetFrequency(const void *);
static void tmrSetFrequency(void *, uint32_t);
static uint32_t tmrGetValue(const void *);
static void tmrSetValue(void *, uint32_t);
/*----------------------------------------------------------------------------*/
const struct TimerClass * const SctSof = &(const struct TimerClass){
    .size = sizeof(struct SctSof),
    .init = tmrInit,
    .deinit = tmrDeinit,

    .enable = tmrEnable,
    .disable = tmrDisable,
    .setAutostop = NULL,
    .setCallback = NULL,
    .getFrequency = tmrGetFrequency,
    .setFrequency = tmrSetFrequency,
    .getOverflow = NULL,
    .setOverflow = NULL,
    .getValue = tmrGetValue,
    .setValue = tmrSetValue
};
/*----------------------------------------------------------------------------*/
static enum SctInput inputToInputChannel(enum SctSofInput input)
{
  switch (input)
  {
    case SCTSOF_I2S0_RX_MWS_6:
    case SCTSOF_I2S0_TX_MWS_6:
      return SCT_INPUT_6;

    case SCTSOF_I2S1_RX_MWS_3:
    case SCTSOF_I2S1_TX_MWS_3:
      return SCT_INPUT_3;

    case SCTSOF_I2S1_RX_MWS_4:
    case SCTSOF_I2S1_TX_MWS_4:
      return SCT_INPUT_4;

    case SCTSOF_USB0_SOF_7:
    case SCTSOF_USB1_SOF_7:
      return SCT_INPUT_7;

    default:
      return SCT_INPUT_NONE;
  }
}
/*----------------------------------------------------------------------------*/
static void interruptHandler(void *object)
{
  struct SctSof * const timer = object;
  const unsigned int part = timer->base.part == SCT_HIGH;
  LPC_SCT_Type * const reg = timer->base.reg;
  const uint32_t evflag = reg->EVFLAG;

  reg->EVFLAG = (1 << timer->i2sEvent) | (1 << timer->usbEvent);

  if (evflag & (1 << timer->i2sEvent))
  {
    const uint16_t value = reg->CAP_PART[timer->i2sEvent][part];

    if (timer->mwsValid)
      timer->mwsDuration = value - timer->mwsPrevious;
    timer->mwsPrevious = value;
    timer->mwsValid = true;
  }

  if (evflag & (1 << timer->usbEvent))
  {
    const uint16_t value = reg->CAP_PART[timer->usbEvent][part];

    if (timer->sofValid)
      timer->sofDuration = value - timer->sofPrevious;
    timer->sofPrevious = value;
    timer->sofValid = true;
  }
}
/*----------------------------------------------------------------------------*/
static void routeGimaInput(enum SctSofInput input)
{
  static const uint8_t channelMuxMap[8] = {
      6, 6, 3, 3, 4, 4, 7, 7
  };
  static const uint8_t functionMuxMap[8] = {
      2, 3, 2, 3, 2, 3, 2, 3
  };

  /* Configure GIMA */
  const uint32_t value = GIMA_SYNCH | GIMA_SELECT(functionMuxMap[input]);
  LPC_GIMA->CTIN_IN[channelMuxMap[input]] = value;
}
/*----------------------------------------------------------------------------*/
static enum Result tmrInit(void *object, const void *configBase)
{
  const struct SctSofConfig * const config = configBase;
  assert(config != NULL);
  assert(config->i2s < SCTSOF_END);
  assert(config->usb < SCTSOF_END);
  assert(config->part != SCT_UNIFIED);

  const struct SctBaseConfig baseConfig = {
      .channel = config->channel,
      .part = config->part
  };
  struct SctSof * const timer = object;
  enum Result res;

  /* Call base class constructor */
  if ((res = SctBase->init(timer, &baseConfig)) != E_OK)
    return res;

  LPC_SCT_Type * const reg = timer->base.reg;
  const unsigned int part = timer->base.part == SCT_HIGH;

  if (!sctAllocateEvent(&timer->base, &timer->i2sEvent))
    return E_BUSY;
  if (!sctAllocateEvent(&timer->base, &timer->usbEvent))
    return E_BUSY;
  timer->base.mask = (1 << timer->i2sEvent) | (1 << timer->usbEvent);

  timer->base.handler = interruptHandler;
  timer->rate = 0;
  timer->mwsDuration = 0;
  timer->mwsPrevious = 0;
  timer->sofDuration = 0;
  timer->sofPrevious = 0;
  timer->i2sInput = inputToInputChannel(config->i2s);
  timer->usbInput = inputToInputChannel(config->usb);
  timer->mwsValid = false;
  timer->sofValid = false;

  routeGimaInput(config->i2s);
  routeGimaInput(config->usb);

  /* Disable the timer before any configuration is done */
  reg->CTRL_PART[part] = CTRL_HALT;

  /* Set desired timer frequency */
  timer->frequency = config->frequency;
  sctSetFrequency(&timer->base, timer->frequency);

  /* Disable match value reload and set current match register value */
  reg->CONFIG |= CONFIG_NORELOAD(part);

  /* Configure I2S event */
  reg->EV[timer->i2sEvent].CTRL =
      (timer->base.part == SCT_HIGH ? EVCTRL_HEVENT : 0)
      // | EVCTRL_OUTSEL_IN // TODO
      | EVCTRL_IOSEL(timer->i2sInput - 1)
      | EVCTRL_IOCOND(IOCOND_RISE)
      | EVCTRL_COMBMODE(COMBMODE_IO)
      | EVCTRL_DIRECTION(DIRECTION_INDEPENDENT);
  reg->EV[timer->i2sEvent].STATE = 0x00000001;
  reg->REGMODE_PART[part] |= 1 << timer->i2sEvent;
  reg->CAPCTRL_PART[timer->i2sEvent][part] = 1 << timer->i2sEvent;

  /* Configure USB event */
  reg->EV[timer->usbEvent].CTRL =
      (timer->base.part == SCT_HIGH ? EVCTRL_HEVENT : 0)
      // | EVCTRL_OUTSEL_IN // TODO
      | EVCTRL_IOSEL(timer->usbInput - 1)
      | EVCTRL_IOCOND(IOCOND_RISE)
      | EVCTRL_COMBMODE(COMBMODE_IO)
      | EVCTRL_DIRECTION(DIRECTION_INDEPENDENT);
  reg->EV[timer->usbEvent].STATE = 0x00000001;
  reg->REGMODE_PART[part] |= 1 << timer->usbEvent;
  reg->CAPCTRL_PART[timer->usbEvent][part] = 1 << timer->usbEvent;

  /* Clear pending requests and enable interrupts */
  reg->EVFLAG = (1 << timer->i2sEvent) | (1 << timer->usbEvent);
  reg->EVEN |= (1 << timer->i2sEvent) | (1 << timer->usbEvent);

  /* Reset current state */
  reg->STATE_PART[part] = 0;

  /* Priority is same for both timer parts*/
  irqSetPriority(timer->base.irq, config->priority);

  /* By default the timer is disabled */
  return E_OK;
}
/*----------------------------------------------------------------------------*/
static void tmrDeinit(void *object)
{
  struct SctSof * const timer = object;
  LPC_SCT_Type * const reg = timer->base.reg;
  const unsigned int part = timer->base.part == SCT_HIGH;

  /* Halt the timer */
  reg->CTRL_PART[part] = CTRL_HALT;
  reg->EVEN &= ~timer->base.mask;
  reg->LIMIT_PART[part] = 0;

  /* Disable interrupt requests */
  reg->EVEN &= ~((1 << timer->i2sEvent) | (1 << timer->usbEvent));

  /* Disable capture events */
  reg->CAPCTRL_PART[timer->i2sEvent][part] = 0;
  reg->CAPCTRL_PART[timer->usbEvent][part] = 0;

  /* Disable allocated SCT events */
  reg->EV[timer->usbEvent].CTRL = 0;
  reg->EV[timer->usbEvent].STATE = 0;
  sctReleaseEvent(&timer->base, timer->usbEvent);
  reg->EV[timer->i2sEvent].CTRL = 0;
  reg->EV[timer->i2sEvent].STATE = 0;
  sctReleaseEvent(&timer->base, timer->i2sEvent);

  /* Reset to default state */
  reg->CONFIG &= ~CONFIG_NORELOAD(part);

  SctBase->deinit(timer);
}
/*----------------------------------------------------------------------------*/
static void tmrEnable(void *object)
{
  struct SctSof * const timer = object;
  LPC_SCT_Type * const reg = timer->base.reg;
  const unsigned int part = timer->base.part == SCT_HIGH;

  timer->mwsDuration = 0;
  timer->sofDuration = 0;
  timer->mwsValid = false;
  timer->sofValid = false;

  reg->EVFLAG = timer->base.mask;
  reg->CTRL_PART[part] &= ~CTRL_HALT;
}
/*----------------------------------------------------------------------------*/
static void tmrDisable(void *object)
{
  struct SctSof * const timer = object;
  LPC_SCT_Type * const reg = timer->base.reg;
  const unsigned int part = timer->base.part == SCT_HIGH;

  reg->CTRL_PART[part] |= CTRL_HALT;
}
/*----------------------------------------------------------------------------*/
static uint32_t tmrGetFrequency(const void *object)
{
  const struct SctSof * const timer = object;
  const LPC_SCT_Type * const reg = timer->base.reg;
  const unsigned int part = timer->base.part == SCT_HIGH;
  const uint32_t baseClock = sctGetClock(&timer->base);
  const uint32_t prescaler = CTRL_PRE_VALUE(reg->CTRL_PART[part]) + 1;

  return baseClock / prescaler;
}
/*----------------------------------------------------------------------------*/
static void tmrSetFrequency(void *object, uint32_t frequency)
{
  struct SctSof * const timer = object;

  timer->frequency = frequency;
  sctSetFrequency(&timer->base, timer->frequency);
}
/*----------------------------------------------------------------------------*/
static uint32_t tmrGetValue(const void *object)
{
  const struct SctSof * const timer = object;
  const LPC_SCT_Type * const reg = timer->base.reg;
  const unsigned int part = timer->base.part == SCT_HIGH;

  return reg->COUNT_PART[part];
}
/*----------------------------------------------------------------------------*/
static void tmrSetValue(void *object, uint32_t value)
{
  struct SctSof * const timer = object;
  LPC_SCT_Type * const reg = timer->base.reg;
  const unsigned int part = timer->base.part == SCT_HIGH;

  assert(value <= UINT16_MAX);

  reg->CTRL_PART[part] |= CTRL_STOP;
  reg->COUNT_PART[part] = value;
  reg->CTRL_PART[part] &= ~CTRL_STOP;
}
/*----------------------------------------------------------------------------*/
uint32_t sctSofGetRatio(const struct SctSof *timer)
{
  assert(timer->frequency < UINT32_MAX / 128);
  assert(timer->frequency % 1000 == 0);

  if (!timer->mwsDuration || !timer->sofDuration || !timer->rate)
    return 0;

  const uint32_t mwsExpected = ((timer->frequency << 7) + (timer->rate >> 1))
      / timer->rate;
  const uint32_t mwsExpectedMul = UINT32_MAX / mwsExpected;
  /* Up to 24 bit with 500 kHz timer frequency */
  const uint32_t sofDurationMul = UINT32_MAX / timer->sofDuration;
  /* Up to 16 bit with 33 MHz timer frequency */
  uint32_t sofExpected = timer->frequency / 1000;

  /* In high-speed mode SOF duration is eight times lower */
  if (timer->sofDuration < (sofExpected >> 2))
    sofExpected >>= 3;

  uint32_t ratio;

  ratio = timer->mwsDuration * sofExpected; /* 32 bit */
  ratio = (uint64_t)ratio * sofDurationMul >> 24;
  ratio = (uint64_t)ratio * mwsExpectedMul >> 24;

  return ratio;
}
/*----------------------------------------------------------------------------*/
void sctSofSetSampleRate(struct SctSof *timer, uint32_t rate)
{
  timer->rate = rate;
}
