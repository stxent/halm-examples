/*
 * stm32f0xx_default/shared/board.h
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef STM32F0XX_DEFAULT_SHARED_BOARD_H_
#define STM32F0XX_DEFAULT_SHARED_BOARD_H_
/*----------------------------------------------------------------------------*/
#include <halm/pin.h>
/*----------------------------------------------------------------------------*/
#define BOARD_LED_0       PIN(PORT_C, 9)
#define BOARD_LED         BOARD_LED_0
#define BOARD_SPI_CS      PIN(PORT_A, 4)
#define BOARD_UART_BUFFER 128
/*----------------------------------------------------------------------------*/
struct Interface;
struct Timer;
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void);
void boardSetupClockPll(void);
struct Interface *boardSetupSerial(void);
struct Interface *boardSetupSpi(void);
struct Timer *boardSetupTimer(void);
/*----------------------------------------------------------------------------*/
#endif /* STM32F0XX_DEFAULT_SHARED_BOARD_H_ */
