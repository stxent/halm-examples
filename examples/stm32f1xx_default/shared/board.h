/*
 * stm32f1xx_default/shared/board.h
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef STM32F1XX_DEFAULT_SHARED_BOARD_H_
#define STM32F1XX_DEFAULT_SHARED_BOARD_H_
/*----------------------------------------------------------------------------*/
#include <halm/generic/work_queue_irq.h>
#include <halm/pin.h>
/*----------------------------------------------------------------------------*/
#define BOARD_BUTTON      PIN(PORT_A, 0)
#define BOARD_LED_0       PIN(PORT_C, 13)
#define BOARD_LED_1       PIN(PORT_C, 14)
#define BOARD_LED_2       PIN(PORT_C, 15)
#define BOARD_LED         BOARD_LED_0
#define BOARD_LED_INV     true
#define BOARD_SDIO_CS     PIN(PORT_A, 4)
#define BOARD_SPI_CS      PIN(PORT_A, 4)
#define BOARD_UART_BUFFER 128

#define BOARD_USB_CDC_INT 0x81
#define BOARD_USB_CDC_RX  0x02
#define BOARD_USB_CDC_TX  0x83
#define BOARD_USB_IND0    BOARD_LED_1
#define BOARD_USB_IND1    BOARD_LED_2

DEFINE_WQ_IRQ(WQ_LP)
/*----------------------------------------------------------------------------*/
struct Entity;
struct Interface;
struct Interrupt;
struct Timer;
struct Watchdog;
/*----------------------------------------------------------------------------*/
size_t boardGetAdcPinCount(void);
void boardSetAdcTimerRate(struct Timer *, size_t, uint32_t);
void boardSetupClockExt(void);
void boardSetupClockPll(void);
void boardSetupLowPriorityWQ(void);
struct Interface *boardSetupAdc(void);
struct Interface *boardSetupAdcDma(void);
struct Timer *boardSetupAdcTimer(void);
struct Interrupt *boardSetupButton(void);
struct Interface *boardSetupCan(struct Timer *);
struct Interface *boardSetupI2C(void);
struct Interface *boardSetupSerial(void);
struct Interface *boardSetupSerialDma(void);
struct Interface *boardSetupSpi(void);
struct Interface *boardSetupSpiSdio(void);
struct Watchdog *boardSetupWdt(bool);
struct Timer *boardSetupTimer(void);
struct Entity *boardSetupUsb(void);
/*----------------------------------------------------------------------------*/
#endif /* STM32F1XX_DEFAULT_SHARED_BOARD_H_ */
