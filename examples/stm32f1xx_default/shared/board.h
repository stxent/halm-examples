/*
 * stm32f1xx_default/shared/board.h
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef STM32F1XX_DEFAULT_SHARED_BOARD_H_
#define STM32F1XX_DEFAULT_SHARED_BOARD_H_
/*----------------------------------------------------------------------------*/
#include <halm/pin.h>
/*----------------------------------------------------------------------------*/
#define BOARD_BUTTON      PIN(PORT_A, 0)
#define BOARD_LED_0       PIN(PORT_C, 13)
#define BOARD_LED_1       PIN(PORT_C, 14)
#define BOARD_LED         BOARD_LED_0
#define BOARD_LED_INV     true
#define BOARD_SDIO_CS     PIN(PORT_A, 4)
#define BOARD_SPI_CS      PIN(PORT_A, 4)
#define BOARD_UART_BUFFER 128
/*----------------------------------------------------------------------------*/
struct Entity;
struct Interface;
struct Interrupt;
struct Timer;
struct Watchdog;
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void);
void boardSetupClockPll(void);
struct Timer *boardSetupAdcTimer(void);
struct Interrupt *boardSetupButton(void);
struct Interface *boardSetupCan(struct Timer *);
struct Watchdog *boardSetupIwdg(void);
struct Interface *boardSetupSerial(void);
struct Interface *boardSetupSerialDma(void);
struct Interface *boardSetupSpi(void);
struct Timer *boardSetupTimer(void);
struct Entity *boardSetupUsb(void);
/*----------------------------------------------------------------------------*/
#endif /* STM32F1XX_DEFAULT_SHARED_BOARD_H_ */
