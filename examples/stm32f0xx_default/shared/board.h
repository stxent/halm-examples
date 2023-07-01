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
#define BOARD_BUTTON      PIN(PORT_A, 0)
#define BOARD_LED_0       PIN(PORT_C, 9)
#define BOARD_LED         BOARD_LED_0
#define BOARD_LED_INV     true
#define BOARD_SPI_CS      PIN(PORT_A, 4)
#define BOARD_UART_BUFFER 128
/*----------------------------------------------------------------------------*/
struct Interface;
struct Interrupt;
struct Timer;
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void);
void boardSetupClockPll(void);
struct Timer *boardSetupAdcTimer(void);
struct Interrupt *boardSetupButton(void);
struct Interface *boardSetupCan(struct Timer *);
struct Interface *boardSetupSerial(void);
struct Interface *boardSetupSerialDma(void);
struct Interface *boardSetupSpi(void);
struct Watchdog *boardSetupWdt(bool);
struct Timer *boardSetupTimer(void);
/*----------------------------------------------------------------------------*/
#endif /* STM32F0XX_DEFAULT_SHARED_BOARD_H_ */
