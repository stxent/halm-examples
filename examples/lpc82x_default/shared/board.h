/*
 * lpc82x_default/shared/board.h
 * Copyright (C) 2025 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef LPC82X_DEFAULT_SHARED_BOARD_H_
#define LPC82X_DEFAULT_SHARED_BOARD_H_
/*----------------------------------------------------------------------------*/
#include <halm/pin.h>
/*----------------------------------------------------------------------------*/
#define BOARD_BUTTON      PIN(0, 17)
#define BOARD_BUTTON_INV  true
#define BOARD_LED_0       PIN(0, 20)
#define BOARD_LED_1       PIN(0, 21)
#define BOARD_LED         BOARD_LED_0
#define BOARD_LED_INV     false
#define BOARD_PWM_0       PIN(0, 24)
#define BOARD_PWM_1       PIN(0, 28)
#define BOARD_PWM_2       PIN(0, 4)
#define BOARD_PWM         BOARD_PWM_0
#define BOARD_SPI_CS      PIN(0, 0)
#define BOARD_UART_BUFFER 128

#define BOARD_MEMCOPY_CH  0
/*----------------------------------------------------------------------------*/
struct Interface;
struct Interrupt;
struct Timer;
struct Watchdog;

struct PwmPackage
{
  struct Timer *timer;
  struct Pwm *output;
  struct Pwm *outputs[3];
};
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void);
void boardSetupClockPll(void);
struct Interrupt *boardSetupBod(void);
struct Interrupt *boardSetupButton(void);
struct Interface *boardSetupFlash(void);
struct PwmPackage boardSetupPwm(bool);
struct Interface *boardSetupSerial(void);
struct Interface *boardSetupSerialDma(void);
struct Interface *boardSetupSerialPoll(void);
struct Timer *boardSetupTimer(void);
struct Timer *boardSetupTimerMRT(void);
struct Timer *boardSetupTimerSCT(void);
struct Timer *boardSetupTimerSCTHigh(void);
struct Timer *boardSetupTimerSCTLow(void);
struct Timer *boardSetupTimerWKT(void);
struct Watchdog *boardSetupWdt(bool);
/*----------------------------------------------------------------------------*/
#endif /* LPC82X_DEFAULT_SHARED_BOARD_H_ */
