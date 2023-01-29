/*
 * lpc11exx_default/shared/board.h
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef LPC11EXX_DEFAULT_SHARED_BOARD_H_
#define LPC11EXX_DEFAULT_SHARED_BOARD_H_
/*----------------------------------------------------------------------------*/
#include <halm/pin.h>
/*----------------------------------------------------------------------------*/
#define BOARD_LED_0 PIN(0, 12)
#define BOARD_LED   BOARD_LED_0
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void);
void boardSetupClockPll(void);
/*----------------------------------------------------------------------------*/
#endif /* LPC11EXX_DEFAULT_SHARED_BOARD_H_ */
