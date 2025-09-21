/*
 * lpc17xx_trace/shared/board.h
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef LPC17XX_TRACE_SHARED_BOARD_H_
#define LPC17XX_TRACE_SHARED_BOARD_H_
/*----------------------------------------------------------------------------*/
#include <halm/generic/work_queue_irq.h>
#include <halm/pin.h>
#include <stddef.h>
/*----------------------------------------------------------------------------*/
#define BOARD_LED_0       PIN(1, 10)
#define BOARD_LED_1       PIN(1, 9)
#define BOARD_LED_2       PIN(1, 8)
#define BOARD_LED         BOARD_LED_0
#define BOARD_LED_INV     false
#define BOARD_SPI0_CS0    PIN(0, 22)
#define BOARD_SPI1_CS0    PIN(0, 6)
#define BOARD_SPI1_CS1    PIN(1, 15)
#define BOARD_SDIO_CS     BOARD_SPI0_CS0
#define BOARD_UART_BUFFER 8192

#define BOARD_USB_IND0    BOARD_LED_1
#define BOARD_USB_IND1    BOARD_LED_2
#define BOARD_USB_CDC_INT 0x81
#define BOARD_USB_CDC_RX  0x02
#define BOARD_USB_CDC_TX  0x82
#define BOARD_USB_MSC_RX  0x02
#define BOARD_USB_MSC_TX  0x82

DEFINE_WQ_IRQ(WQ_LP)
/*----------------------------------------------------------------------------*/
struct Entity;
struct Interface;
struct Timer;
struct Usb;
/*----------------------------------------------------------------------------*/
void boardSetupClockExt(void);
void boardSetupClockPll(void);
void boardSetupLowPriorityWQ(void);
struct Timer *boardSetupAdcTimer(void);
struct Interface *boardSetupSdio(bool, struct Timer *);
struct Interface *boardSetupSerial(void);
struct Interface *boardSetupSpi(void);
struct Interface *boardSetupSpi0(void);
struct Interface *boardSetupSpi1(void);
struct Interface *boardSetupSpiSdio(void);
struct Timer *boardSetupTimer(void);
struct Timer *boardSetupTimer0(void);
struct Timer *boardSetupTimer1(void);
struct Timer *boardSetupTimer2(void);
struct Timer *boardSetupTimerAux(void);
struct Usb *boardSetupUsb(void);
/*----------------------------------------------------------------------------*/
#endif /* LPC17XX_TRACE_SHARED_BOARD_H_ */
