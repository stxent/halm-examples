/*
 * m48x_default/shared/board.h
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef M48X_DEFAULT_SHARED_BOARD_H_
#define M48X_DEFAULT_SHARED_BOARD_H_
/*----------------------------------------------------------------------------*/
#include <halm/pin.h>
#include <stddef.h>
/*----------------------------------------------------------------------------*/
#define BOARD_BPWM_0      PIN(PORT_A, 3)
#define BOARD_BPWM_1      PIN(PORT_A, 4)
#define BOARD_BPWM        BOARD_BPWM_0
#define BOARD_BUTTON_0    PIN(PORT_G, 15)
#define BOARD_BUTTON_1    PIN(PORT_F, 11)
#define BOARD_BUTTON      BOARD_BUTTON_0
#define BOARD_LED_0       PIN(PORT_H, 0)
#define BOARD_LED_1       PIN(PORT_H, 1)
#define BOARD_LED_2       PIN(PORT_H, 2)
#define BOARD_LED         BOARD_LED_0
#define BOARD_LED_INV     true
#define BOARD_SPI_CS      PIN(PORT_C, 9)
#define BOARD_QSPI_CS     PIN(PORT_C, 3)
#define BOARD_UART_BUFFER 512

#define BOARD_USB_IND0    BOARD_LED_1
#define BOARD_USB_IND1    BOARD_LED_2
#define BOARD_USB_CDC_INT 0x81
#define BOARD_USB_CDC_RX  0x02
#define BOARD_USB_CDC_TX  0x82
#define BOARD_USB_MSC_RX  0x01
#define BOARD_USB_MSC_TX  0x81
/*----------------------------------------------------------------------------*/
struct Entity;
struct Interface;
struct Interrupt;
struct Pwm;
struct Timer;
struct Watchdog;

struct PwmPackage
{
  struct Timer *timer;
  struct Pwm *output;
  struct Pwm *outputs[3];
};
/*----------------------------------------------------------------------------*/
size_t boardGetAdcPinCount(void);
void boardSetAdcTimerRate(struct Timer *, size_t, uint32_t);
void boardSetupClockExt(void);
void boardSetupClockPll(void);
struct Interface *boardSetupAdc(void);
struct Interface *boardSetupAdcDma(void);
struct Timer *boardSetupAdcTimer(void);
struct Interrupt *boardSetupButton(void);
struct Interface *boardSetupCan(struct Timer *);
struct Interface *boardSetupFlash(void);
struct Interface *boardSetupI2C(void);
struct PwmPackage boardSetupPwm(bool);
struct PwmPackage boardSetupPwmBPWM(bool);
struct PwmPackage boardSetupPwmEPWM(bool);
struct Interface *boardSetupQspi(void);
struct Interface *boardSetupSdio(bool, struct Timer *);
struct Interface *boardSetupSerial(void);
struct Interface *boardSetupSpi(void);
struct Interface *boardSetupSpiDma(void);
struct Timer *boardSetupTimer(void);
struct Entity *boardSetupUsb(void);
struct Entity *boardSetupUsbFs(void);
struct Entity *boardSetupUsbHs(void);
struct Watchdog *boardSetupWdt(bool);
/*----------------------------------------------------------------------------*/
#endif /* M48X_DEFAULT_SHARED_BOARD_H_ */
