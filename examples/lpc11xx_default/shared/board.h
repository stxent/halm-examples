/*
 * lpc11xx_default/shared/board.h
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef LPC11XX_DEFAULT_SHARED_BOARD_H_
#define LPC11XX_DEFAULT_SHARED_BOARD_H_
/*----------------------------------------------------------------------------*/
#include <halm/pin.h>
#include <stddef.h>
/*----------------------------------------------------------------------------*/
#define BOARD_BUTTON      PIN(1, 8)
#define BOARD_BUTTON_INV  true
#define BOARD_LED_0       PIN(1, 2)
#define BOARD_LED         BOARD_LED_0
#define BOARD_LED_INV     false
#define BOARD_SPI_CS      PIN(0, 3)
#define BOARD_UART_BUFFER 128
/*----------------------------------------------------------------------------*/
struct Interface;
struct Interrupt;
struct Timer;
struct Watchdog;
/*----------------------------------------------------------------------------*/
size_t boardGetAdcPinCount(void);
void boardSetAdcTimerRate(struct Timer *, size_t, uint32_t);
void boardSetupClockExt(void);
void boardSetupClockPll(void);
struct Interface *boardSetupAdc(void);
struct Interface *boardSetupAdcOneShot(void);
struct Timer *boardSetupAdcTimer(void);
struct Interrupt *boardSetupBod(void);
struct Interrupt *boardSetupButton(void);
struct Interface *boardSetupFlash(void);
struct Interface *boardSetupI2C(void);
struct Interface *boardSetupSerial(void);
struct Interface *boardSetupSpi(void);
struct Timer *boardSetupTimer(void);
struct Timer *boardSetupTimer16B0(void);
struct Timer *boardSetupTimer16B1(void);
struct Timer *boardSetupTimer32B0(void);
struct Timer *boardSetupTimer32B1(void);
struct Watchdog *boardSetupWdt(bool);
/*----------------------------------------------------------------------------*/
#endif /* LPC11XX_DEFAULT_SHARED_BOARD_H_ */
