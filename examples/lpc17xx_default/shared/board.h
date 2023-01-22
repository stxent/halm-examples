/*
 * lpc17xx_default/shared/board.h
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef LPC17XX_DEFAULT_SHARED_BOARD_H_
#define LPC17XX_DEFAULT_SHARED_BOARD_H_
/*----------------------------------------------------------------------------*/
#include <halm/pin.h>
#include <stddef.h>
/*----------------------------------------------------------------------------*/
#define BOARD_LED_0       PIN(1, 8)
#define BOARD_LED_1       PIN(1, 9)
#define BOARD_LED_2       PIN(1, 10)
#define BOARD_LED         BOARD_LED_0
#define BOARD_SDIO_CS     PIN(0, 22)
#define BOARD_SPI_CS      PIN(0, 16)
#define BOARD_UART_BUFFER 128
/*----------------------------------------------------------------------------*/
struct Entity;
struct Interface;
struct Timer;
struct Watchdog;
/*----------------------------------------------------------------------------*/
size_t boardGetAdcPinCount(void);
void boardSetupClockExt(void);
void boardSetupClockPll(void);
struct Interface *boardSetupAdc(void);
struct Interface *boardSetupAdcDma(void);
struct Interface *boardSetupAdcOneShot(void);
struct Interface *boardSetupAdcStream(void);
struct Timer *boardSetupAdcTimer(void);
struct Interface *boardSetupCan(struct Timer *);
struct Interface *boardSetupDac(void);
struct Interface *boardSetupDacDma(void);
struct Interface *boardSetupI2C(void);
struct Interface *boardSetupI2S(void);
struct Interface *boardSetupSerial(void);
struct Interface *boardSetupSerialDma(void);
struct Interface *boardSetupSpi(void);
struct Interface *boardSetupSpiDma(void);
struct Timer *boardSetupTimer(void);
struct Entity *boardSetupUsb(void);
struct Watchdog *boardSetupWdt(void);
/*----------------------------------------------------------------------------*/
#endif /* LPC17XX_DEFAULT_SHARED_BOARD_H_ */
