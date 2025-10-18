/*
 * lpc82x_default/shared/board.c
 * Copyright (C) 2025 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#include "board.h"
#include <halm/platform/lpc/clocking.h>
#include <halm/platform/lpc/bod.h>
#include <halm/platform/lpc/flash.h>
#include <halm/platform/lpc/mrt.h>
#include <halm/platform/lpc/pin_int.h>
#include <halm/platform/lpc/sct_pwm.h>
#include <halm/platform/lpc/sct_timer.h>
#include <halm/platform/lpc/serial.h>
#include <halm/platform/lpc/serial_dma.h>
#include <halm/platform/lpc/serial_poll.h>
#include <halm/platform/lpc/wkt.h>
#include <halm/platform/lpc/wwdt.h>
#include <assert.h>
/*----------------------------------------------------------------------------*/
[[gnu::alias("boardSetupTimerMRT")]] struct Timer *boardSetupTimer(void);
/*----------------------------------------------------------------------------*/
static const struct ExternalOscConfig extOscConfig = {
    .frequency = 12000000
};
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void)
{
  static const struct GenericClockConfig mainClockConfigExt = {
      .divisor = 1,
      .source = CLOCK_EXTERNAL
  };

  clockEnable(LowPowerOsc, NULL);
  while (!clockReady(LowPowerOsc));

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(MainClock, &mainClockConfigExt);
}
/*----------------------------------------------------------------------------*/
void boardSetupClockPll(void)
{
  static const struct PllConfig sysPllConfig = {
      .divisor = 4,
      .multiplier = 20,
      .source = CLOCK_EXTERNAL
  };
  static const struct GenericClockConfig mainClockConfigPll = {
      .divisor = 2,
      .source = CLOCK_PLL
  };

  clockEnable(LowPowerOsc, NULL);
  while (!clockReady(LowPowerOsc));

  clockEnable(ExternalOsc, &extOscConfig);
  while (!clockReady(ExternalOsc));

  clockEnable(SystemPll, &sysPllConfig);
  while (!clockReady(SystemPll));

  clockEnable(MainClock, &mainClockConfigPll);
}
/*----------------------------------------------------------------------------*/
struct Interrupt *boardSetupBod(void)
{
  static const struct BodConfig bodConfig = {
      .eventLevel = BOD_EVENT_2V85,
      .resetLevel = BOD_RESET_DISABLED
  };

  struct Interrupt * const interrupt = init(Bod, &bodConfig);
  assert(interrupt != NULL);
  return interrupt;
}
/*----------------------------------------------------------------------------*/
struct Interrupt *boardSetupButton(void)
{
  static const struct PinIntConfig buttonIntConfig = {
      .pin = BOARD_BUTTON,
      .event = BOARD_BUTTON_INV ? INPUT_FALLING : INPUT_RISING,
      .pull = BOARD_BUTTON_INV ? PIN_PULLUP : PIN_PULLDOWN
  };

  struct Interrupt * const interrupt = init(PinInt, &buttonIntConfig);
  assert(interrupt != NULL);
  return interrupt;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupFlash(void)
{
  struct Interface * const interface = init(Flash, NULL);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct PwmPackage boardSetupPwm(bool centered)
{
  const struct SctPwmUnitConfig pwmTimerConfig = {
      .frequency = 1000000,
      .resolution = 20000,
      .part = SCT_LOW,
      .channel = 0,
      .centered = centered
  };
  const bool inversion = false;

  struct SctPwmUnit * const timer = init(SctPwmUnit, &pwmTimerConfig);
  assert(timer != NULL);

  struct Pwm * const pwm0 = sctPwmCreate(timer, BOARD_PWM_0, inversion);
  assert(pwm0 != NULL);
  struct Pwm * const pwm1 = sctPwmCreate(timer, BOARD_PWM_1, inversion);
  assert(pwm1 != NULL);
  struct Pwm * const pwm2 = sctPwmCreateDoubleEdge(timer, BOARD_PWM_2,
      inversion);
  assert(pwm2 != NULL);

  return (struct PwmPackage){
      (struct Timer *)timer,
      pwm0,
      {pwm0, pwm1, pwm2}
  };
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerial(void)
{
  static const struct SerialConfig serialConfig = {
      .rxLength = BOARD_UART_BUFFER,
      .txLength = BOARD_UART_BUFFER,
      .rate = 19200,
      .rx = PIN(0, 26),
      .tx = PIN(0, 25),
      .channel = 0
  };

  struct Interface * const interface = init(Serial, &serialConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerialDma(void)
{
  static const struct SerialDmaConfig serialDmaConfig = {
      .rxChunks = 8,
      .rxLength = BOARD_UART_BUFFER,
      .txLength = BOARD_UART_BUFFER,
      .rate = 19200,
      .rx = PIN(0, 26),
      .tx = PIN(0, 25),
      .channel = 0
  };

  struct Interface * const interface = init(SerialDma, &serialDmaConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Interface *boardSetupSerialPoll(void)
{
  static const struct SerialPollConfig serialPollConfig = {
      .rate = 19200,
      .rx = PIN(0, 26),
      .tx = PIN(0, 25),
      .channel = 0
  };

  struct Interface * const interface = init(SerialPoll, &serialPollConfig);
  assert(interface != NULL);
  return interface;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimerMRT(void)
{
  static const struct MrtConfig mrtConfig = {
      .channel = 0
  };

  struct Timer * const timer = init(Mrt, &mrtConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimerSCT(void)
{
  static const struct SctTimerConfig timerConfig = {
      .frequency = 1000000,
      .output = SCT_OUTPUT_3,
      .part = SCT_UNIFIED,
      .channel = 0
  };

  struct Timer * const timer = init(SctUnifiedTimer, &timerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimerSCTHigh(void)
{
  static const struct SctTimerConfig timerConfig = {
      .frequency = 1000000,
      .output = SCT_OUTPUT_3,
      .part = SCT_HIGH,
      .channel = 0
  };

  struct Timer * const timer = init(SctTimer, &timerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimerSCTLow(void)
{
  static const struct SctTimerConfig timerConfig = {
      .frequency = 1000000,
      .part = SCT_LOW,
      .channel = 0
  };

  struct Timer * const timer = init(SctTimer, &timerConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Timer *boardSetupTimerWKT(void)
{
  static const struct WktConfig wktConfig = {
      .pin = 0,
      .source = WKT_CLOCK_LOW_POWER
  };

  struct Timer * const timer = init(Wkt, &wktConfig);
  assert(timer != NULL);
  return timer;
}
/*----------------------------------------------------------------------------*/
struct Watchdog *boardSetupWdt(bool disarmed)
{
  /* Clocks */
  static const struct WdtOscConfig wdtOscConfig = {
      .frequency = WDT_FREQ_1050
  };

  /* Objects */
  const struct WwdtConfig wwdtConfig = {
      .period = 5000,
      .window = 0,
      .disarmed = disarmed
  };

  clockEnable(WdtOsc, &wdtOscConfig);
  while (!clockReady(WdtOsc));

  struct Watchdog * const timer = init(Wwdt, &wwdtConfig);
  assert(timer != NULL);
  return timer;
}
