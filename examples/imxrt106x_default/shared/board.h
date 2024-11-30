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
#define BOARD_LED_1       PIN(PORT_B0, 2)
#define BOARD_LED_2       PIN(PORT_B0, 1)
#define BOARD_LED         BOARD_LED_0
#define BOARD_LED_INV     false
#define BOARD_UART_BUFFER 512

#define BOARD_USB_IND0    BOARD_LED_1
#define BOARD_USB_IND1    BOARD_LED_2
#define BOARD_USB_CDC_INT 0x82
#define BOARD_USB_CDC_RX  0x01
#define BOARD_USB_CDC_TX  0x81
#define BOARD_USB_MSC_RX  0x01
#define BOARD_USB_MSC_TX  0x81

#define BOARD_MEMCOPY_CH  15
/*----------------------------------------------------------------------------*/
struct Entity;
struct Interface;
struct Timer;
struct Usb;
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void);
void boardSetupClockInt(void);
void boardSetupClockPll(void);
void boardSetupClockPll1(void);
void boardSetupClockPll2Pfd0(void);
void boardSetupClockPll3(void);
struct Interface *boardSetupSerial(void);
struct Interface *boardSetupSerialDma(void);
struct Timer *boardSetupTimer(void);
struct Timer *boardSetupTimerPIT(void);
struct Timer *boardSetupTimerPIT0(void);
struct Timer *boardSetupTimerPIT2(void);
struct Timer *boardSetupTimerPIT3(void);
struct Usb *boardSetupUsb(void);
struct Usb *boardSetupUsb1(void);
struct Usb *boardSetupUsb2(void);
/*----------------------------------------------------------------------------*/
#endif /* IMXRT106X_DEFAULT_SHARED_BOARD_H_ */
