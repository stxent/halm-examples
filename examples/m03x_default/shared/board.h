/*
 * m03x_default/shared/board.h
 * Copyright (C) 2023 xent
 * Project is distributed under the terms of the GNU General Public License v3.0
 */

#ifndef M03X_DEFAULT_SHARED_BOARD_H_
#define M03X_DEFAULT_SHARED_BOARD_H_
/*----------------------------------------------------------------------------*/
#include <halm/pin.h>
#include <stddef.h>
/*----------------------------------------------------------------------------*/
#define BOARD_BPWM_0      PIN(PORT_A, 3)
#define BOARD_BPWM_1      PIN(PORT_A, 4)
#define BOARD_BPWM        BOARD_BPWM_0
#define BOARD_BUTTON      PIN(PORT_B, 15)
#define BOARD_BUTTON_INV  true
#define BOARD_LED_0       PIN(PORT_B, 14)
#define BOARD_LED_1       PIN(PORT_C, 6)
#define BOARD_LED_2       PIN(PORT_C, 7)
#define BOARD_LED         BOARD_LED_0
#define BOARD_LED_INV     true
#define BOARD_SPI_CS      PIN(PORT_B, 10)
#define BOARD_UART_BUFFER 128

#define BOARD_USB_IND0    BOARD_LED_1
#define BOARD_USB_IND1    BOARD_LED_2
#define BOARD_USB_CDC_INT 0x81
#define BOARD_USB_CDC_RX  0x02
#define BOARD_USB_CDC_TX  0x82
/*----------------------------------------------------------------------------*/
struct Entity;
struct Interface;
struct Interrupt;
struct Pwm;
struct Timer;
struct Usb;
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
struct Interface *boardSetupFlash(void);
struct Interface *boardSetupI2C(void);
struct PwmPackage boardSetupPwm(bool);
struct PwmPackage boardSetupPwmBPWM(bool);
struct PwmPackage boardSetupPwmEPWM(bool);
struct Interface *boardSetupSerial(void);
struct Interface *boardSetupSerialDma(void);
struct Interface *boardSetupSerialDmaTOC(void);
struct Interface *boardSetupSpi(void);
struct Interface *boardSetupSpiDma(void);
struct Timer *boardSetupTimer(void);
struct Usb *boardSetupUsb(void);
struct Watchdog *boardSetupWdt(bool);
/*----------------------------------------------------------------------------*/
#endif /* M03X_DEFAULT_SHARED_BOARD_H_ */
