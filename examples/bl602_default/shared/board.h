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
#define BOARD_LED_1       PIN(0, 1)
#define BOARD_LED_2       PIN(0, 5)
#define BOARD_LED         BOARD_LED_0
#define BOARD_LED_INV     false
#define BOARD_UART_BUFFER 128
/*----------------------------------------------------------------------------*/
struct Interface;
struct Timer;
struct Timer64;
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void);
void boardSetupClockPll(void);
struct Interface *boardSetupSerial(void);
struct Timer *boardSetupTimer(void);
struct Timer64 *boardSetupTimer64(void);
/*----------------------------------------------------------------------------*/
#endif /* BL602_DEFAULT_SHARED_BOARD_H_ */
