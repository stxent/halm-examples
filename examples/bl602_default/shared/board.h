/*
 * bl602_default/shared/board.h
 * Copyright (C) 2026 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef BL602_DEFAULT_SHARED_BOARD_H_
#define BL602_DEFAULT_SHARED_BOARD_H_
/*----------------------------------------------------------------------------*/
#include <halm/pin.h>
/*----------------------------------------------------------------------------*/
#define BOARD_LED_0       PIN(0, 3)
#define BOARD_LED         BOARD_LED_0
#define BOARD_LED_INV     false
#define BOARD_PWM_0       PIN(0, 4) /* Channel 4 */
#define BOARD_PWM_1       PIN(0, 3) /* Channel 3 */
#define BOARD_PWM_2       PIN(0, 16) /* Channel 1 */
#define BOARD_SPI_CS      PIN(0, 16)
#define BOARD_UART_BUFFER 128
/*----------------------------------------------------------------------------*/
struct Interface;
struct Timer;
struct Timer64;

struct PwmPackage
{
  struct Timer *timer;
  struct Timer *timers[3];
  struct Pwm *output;
  struct Pwm *outputs[3];
};
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void);
void boardSetupClockPll(void);
struct Interface *boardSetupI2C(void);
struct PwmPackage boardSetupPwm(bool);
struct Interface *boardSetupSerial(void);
struct Interface *boardSetupSerialDma(void);
struct Interface *boardSetupSpi(void);
struct Interface *boardSetupSpiDma(void);
struct Timer *boardSetupTimer(void);
struct Timer64 *boardSetupTimer64(void);
/*----------------------------------------------------------------------------*/
#endif /* BL602_DEFAULT_SHARED_BOARD_H_ */
