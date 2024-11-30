/*
 * lpc43xx_m0app/shared/board.h
 * Copyright (C) 2024 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef LPC43XX_M0APP_SHARED_BOARD_H_
#define LPC43XX_M0APP_SHARED_BOARD_H_
/*----------------------------------------------------------------------------*/
#include <halm/pin.h>
/*----------------------------------------------------------------------------*/
#define BOARD_LED_0       PIN(PORT_5, 7)
#define BOARD_LED_1       PIN(PORT_5, 5)
#define BOARD_LED_2       PIN(PORT_4, 0)
#define BOARD_LED         BOARD_LED_0
#define BOARD_LED_INV     false
#define BOARD_UART_BUFFER 512

#define BOARD_USB_IND0    BOARD_LED_1
#define BOARD_USB_IND1    BOARD_LED_2
#define BOARD_USB_CDC_INT 0x81
#define BOARD_USB_CDC_RX  0x02
#define BOARD_USB_CDC_TX  0x83
/*----------------------------------------------------------------------------*/
struct Entity;
struct Interface;
struct Timer;
struct Usb;
/*----------------------------------------------------------------------------*/
void boardSetupClock(void);
void boardSetupClockExt(void);
void boardSetupClockPll(void);
struct Interface *boardSetupSerial(void);
struct Timer *boardSetupTimer(void);
struct Usb *boardSetupUsb(void);
/*----------------------------------------------------------------------------*/
#endif /* LPC43XX_M0APP_SHARED_BOARD_H_ */
