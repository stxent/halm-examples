/*
 * imxrt106x_default/shared/board.h
 * Copyright (C) 2024 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef IMXRT106X_DEFAULT_SHARED_BOARD_H_
#define IMXRT106X_DEFAULT_SHARED_BOARD_H_
/*----------------------------------------------------------------------------*/
#include <halm/pin.h>
/*----------------------------------------------------------------------------*/
#define BOARD_LED_0       PIN(PORT_B0, 3)
#define BOARD_LED         BOARD_LED_0
#define BOARD_LED_INV     false
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void);
void boardSetupClockPll(void);
void boardSetupClockPll1(void);
void boardSetupClockPll2Pfd0(void);
/*----------------------------------------------------------------------------*/
#endif /* IMXRT106X_DEFAULT_SHARED_BOARD_H_ */
